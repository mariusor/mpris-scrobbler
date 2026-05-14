/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_CURL_H
#define MPRIS_SCROBBLER_CURL_H

#include <curl/curl.h>

#ifdef RETRY_ENABLED

#define MAX_RETRIES 5

static void retry_cb(int fd, short kind, void *data)
{
    assert(data);

    const struct scrobbler_connection *conn = data;
    assert(conn->parent);
    assert(conn->parent->handle);
    assert(conn->handle);
    curl_multi_add_handle(conn->parent->handle, conn->handle);
}

static void connection_retry(struct scrobbler_connection *conn)
{
    const struct timeval retry_timeout = { .tv_sec = 3 + conn->retries, .tv_usec = 0, };
    http_response_clean(&conn->response);

    const struct scrobbler *s = conn->parent;
    curl_multi_remove_handle(conn->parent->handle, conn->handle);

    if (conn->retries == 0) {
        evtimer_assign(&conn->retry_event, s->evbase, retry_cb, conn);
    } else {
        evtimer_del(&conn->retry_event);
    }
    evtimer_add(&conn->retry_event, &retry_timeout);
    conn->retries++;
    _debug("curl::retrying[%zd]: in %2.2lfs", conn->retries, timeval_to_seconds(retry_timeout));
}

static bool connection_should_retry(const struct scrobbler_connection *conn)
{
    const int code = (int)conn->response.code;
    if (code >= 400 && code < 500) {
        // NOTE(marius): 4XX errors mean that there's something wrong with our request
        // so we shouldn't retry.
        return false;
    }
    return conn->retries < MAX_RETRIES;
}
#endif

static bool connection_was_fulfilled(const struct scrobbler_connection *);
/*
 * Based on https://curl.se/libcurl/c/hiperfifo.html
 * Check for completed transfers, and remove their easy handles
 */
static void check_multi_info(struct scrobbler *s)
{
    char *eff_url;
    int msgs_left;
    struct scrobbler_connection *conn;
    CURLMsg *msg;
    long code = -1;
    _trace2("curl::check_multi_info[%p]: remaining %d", s, s->still_running);

    while((msg = curl_multi_info_read(s->handle, &msgs_left))) {
        if(msg->msg != CURLMSG_DONE) {
            continue;
        }

        CURL *easy = msg->easy_handle;
        const CURLcode res = msg->data.result;
        curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
        curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &code);

        assert(conn);

        if (strlen(conn->error) != 0) {
            _warn("curl::transfer::done[%zd]: %s => (%d) %s", conn->idx, eff_url, res, conn->error);
        } else {
            _trace("curl::transfer::done[%zd]: %s", conn->idx, eff_url);
            conn->response.code = code;
        }

        http_response_print(&conn->response, log_tracing2);

        const bool success = conn->response.code == 200;
        _info(" api::submitted_to[%s]: %s", get_api_type_label(conn->credentials.end_point), (success ? "ok" : "nok"));
        if(evtimer_pending(&s->timer_event, NULL)) {
            _trace2("curl::multi_timer_remove(%p)", &s->timer_event);
            evtimer_del(&s->timer_event);
        }
#ifdef RETRY_ENABLED
        if (!connection_was_fulfilled(conn) && connection_should_retry(conn)) {
            connection_retry(conn);
            return
        }
#endif
        conn->should_free = connection_was_fulfilled(conn);
    }
}

static void scrobbler_connections_clean(struct scrobble_connections*, const bool);

/*
 * Based on https://curl.se/libcurl/c/hiperfifo.html
 * Called by libevent when our timeout expires
 */
static void timer_cb(int fd, short kind, void *data)
{
    assert(data);

    struct scrobbler *s = data;

    const CURLMcode rc = curl_multi_socket_action(s->handle, CURL_SOCKET_TIMEOUT, 0, &s->still_running);
    const char *res;
    switch(rc) {
    case CURLM_OK:
        res = "OK";
        break;
    case CURLM_BAD_HANDLE:
        res = "bad multi handle";
        break;
    case CURLM_BAD_EASY_HANDLE:
        res = "bad easy handle";
        break;
    case CURLM_OUT_OF_MEMORY:
        res = "OOM";
        break;
    case CURLM_INTERNAL_ERROR:
        res = "internal error";
        break;
    case CURLM_UNKNOWN_OPTION:
        res = "unknown option";
        break;
    case CURLM_LAST:
        res = "last";
        break;
    case CURLM_BAD_SOCKET:
        _warn("curl::multi_socket_activation:error: %s", curl_multi_strerror(rc));
        return;
    default:
        res = "unknown";
    }
    _trace2("curl::multi_socket_activation:%s[%p:%d]: still_running: %d", res, s, fd, s->still_running);

    check_multi_info(s);

    scrobbler_connections_clean(&s->connections, false);
}

/* Called by libevent when we get action on a multi socket */
static void event_cb(int fd, short kind, void *data)
{
    assert(data);

    struct scrobbler *s = (struct scrobbler*)data;
    assert(&s->connections);

    const int action = ((kind & EV_READ) ? CURL_CSELECT_IN : 0) | ((kind & EV_WRITE) ? CURL_CSELECT_OUT : 0);

    const CURLMcode rc = curl_multi_socket_action(s->handle, fd, action, &s->still_running);
    if (rc != CURLM_OK) {
        _warn("curl::transfer::error: %s", curl_multi_strerror(rc));
        return;
    }

    _trace2("curl::event_cb(%p:%p:%zd:%zd): still running: %d", s, s->handle, fd, action, s->still_running);

    check_multi_info(s);

    scrobbler_connections_clean(&s->connections, false);
}

struct sock_info {
    struct event ev;
    CURL *handle;
    struct scrobbler *parent;
    long timeout;
    curl_socket_t sockfd;
    int action;
};

/*
 * Based on https://curl.se/libcurl/c/hiperfifo.html
 * Assign information to a scrobbler_connection structure
 */
static void setsock(struct sock_info *info, curl_socket_t sock, CURL *e, int act, struct scrobbler *parent)
{
    const short kind = ((act & CURL_POLL_IN) ? EV_READ : 0) | ((act & CURL_POLL_OUT) ? EV_WRITE : 0) | EV_PERSIST;
    info->handle = e;
    info->sockfd = sock;
    info->action = act;

    if (event_initialized(&info->ev)) {
        event_del(&info->ev);
    }

    evutil_make_socket_nonblocking(info->sockfd);
    event_assign(&info->ev, parent->evbase, info->sockfd, kind, event_cb, parent);
    event_add(&info->ev, NULL);
}

static void remsock(struct sock_info *info)
{
    if(event_initialized(&info->ev)) {
        event_del(&info->ev);
    }
    free(info);
}

static struct sock_info* addsock(curl_socket_t sock, CURL *e, int act, struct scrobbler *parent)
{
    struct sock_info *info = calloc(1, sizeof(struct sock_info));

    info->parent = parent;
    setsock(info, sock, e, act, parent);
    curl_multi_assign(parent->handle, sock, info);

    return info;
}

const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE" };

/* CURLMOPT_SOCKETFUNCTION */
static int curl_request_has_data(CURL *e, const curl_socket_t sock, int what, void *data, void *sock_data)
{
    assert(e);
    if (NULL == data) { return -1; }

    struct scrobbler *s = data;
    int events = 0;

    _trace2("curl::data_callback[%p:%zd]: s: %p, conn: %p", e, sock, data, sock_data);

    struct sock_info *info = sock_data;
    bool missing_connection = (NULL == info);
    switch(what) {
    case CURL_POLL_REMOVE:
        if (!missing_connection) {
            _trace2("curl::data_remove[%p]: action=%s", e, whatstr[what]);
            remsock((info));
        }
        break;
    case CURL_POLL_IN:
    case CURL_POLL_OUT:
    case CURL_POLL_INOUT:
        if (missing_connection) {
            info = addsock(sock, e, what, s);
        } else {
            // set libevent context for a new connection
            if(what != CURL_POLL_IN) events |= EV_WRITE;
            if(what != CURL_POLL_OUT) events |= EV_READ;

            events |= EV_PERSIST;
            setsock(info, sock, e, what, s);
        }
        if (info->action != what) {
            _trace2("curl::data_callback[%p]: s=%d, changing action=%s->%s",  e, sock, whatstr[info->action], whatstr[what]);
        } else {
            _trace2("curl::data_callback[%p]: s=%d, action=%s",  e, sock, whatstr[what]);
        }
        break;
    default:
        _trace2("curl::unknown_socket_action[%p:%d]: action=%s", e, events, whatstr[what]);
        assert(false);
    }

    return 0;
}

/* Update the event timer after curl_multi library calls */
static int curl_request_wait_timeout(CURLM *multi, long timeout_ms, void *data)
{
    assert(multi);
    if (NULL == data) { return -1; }

    struct scrobbler *s = data;

    const struct timeval timeout = {
        .tv_sec = timeout_ms/1000,
        .tv_usec = (timeout_ms % 1000) * 1000,
    };

    /**
     * if timeout_ms is -1, just delete the timer
     *
     * For all other values of timeout_ms, this should set or *update*
     * the timer to the new value
     */
    if (timeout_ms == -1) {
        _trace2("curl::multi_timer_remove(%p:%p)", multi, s->timer_event);
        evtimer_del(&s->timer_event);
    } else {
        _trace2("curl::multi_timer_update(%p:%p):still_running: %d, timeout: %d", s, &s->timer_event, s->still_running, timeout_ms);
        evtimer_add(&s->timer_event, &timeout);
    }

    return 0;
}

static size_t http_response_write_body(void *buffer, size_t size, size_t nmemb, void* data)
{
    if (NULL == buffer) { return 0; }
    if (NULL == data) { return 0; }
    if (0 == size) { return 0; }
    if (0 == nmemb) { return 0; }


    struct scrobbler_connection *conn = (struct scrobbler_connection*)data;
    struct http_response *res = &conn->response;
    assert(res);

    const size_t new_size = size * nmemb;

    strncat(res->body, buffer, new_size);
    res->body_length += new_size;

    assert (res->body_length < MAX_BODY_SIZE);
    memset(res->body + res->body_length, 0x0, MAX_BODY_SIZE - res->body_length);

    return new_size;
}

static size_t http_response_write_headers(char *buffer, size_t size, size_t nitems, void* data)
{
    if (NULL == buffer) { return 0; }
    if (NULL == data) { return 0; }
    if (0 == size) { return 0; }
    if (0 == nitems) { return 0; }

    struct scrobbler_connection *conn = (struct scrobbler_connection *)data;
    struct http_response *res = &conn->response;

    const size_t new_size = size * nitems;

    struct http_header *h = http_header_new();

    http_header_load(buffer, nitems, h);
    if (_is_zero(h->name)) { goto _err_exit; }

    arrput(res->headers, h);
    return new_size;

_err_exit:
    http_header_free(h);
    return new_size;
}

#if defined(LIBCURL_DEBUG) && LIBCURL_DEBUG
static char tbuff[8192];
static int curl_debug(CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
{
    memset(tbuff, 0, 8192);

    const char *act;
    switch(type) {
    case CURLINFO_HEADER_OUT:
        act = "send header";
        memcpy(tbuff, data, max(0, size-2));
        break;
    case CURLINFO_DATA_OUT:
        act = "send data";
        memcpy(tbuff, data, size);
        break;
    case CURLINFO_HEADER_IN:
        act = "recv header";
        memcpy(tbuff, data, max(0, size-2));
        break;
    case CURLINFO_DATA_IN:
        act = "recv data";
        memcpy(tbuff, data, size);
        break;
    case CURLINFO_SSL_DATA_OUT:
        act = "send SSL data";
        break;
    case CURLINFO_SSL_DATA_IN:
        act = "recv SSL data";
        break;
    case CURLINFO_TEXT:
    default: /* in case a new one is introduced to shock us */
        act = "";
        break;
    }
    if (strlen(act) > 0 ) {
        if (strlen(tbuff) > 0) {
            _trace2("curl::debug[%p]: %s %s", handle, act, tbuff);
        } else {
            _trace2("curl::debug[%p]: %s", handle, act);
        }
    }

    return 0;
}
#endif

static void curl_easy_handle_init(struct scrobbler_connection *conn)
{
    conn->handle = curl_easy_init();
}

static void curl_easy_handle_cleanup(struct scrobbler_connection *conn)
{
    if (NULL == conn) return;

    if (NULL != conn->handle) {
        _trace2("scrobbler::connection_free:curl_easy_handle[%p]", conn->handle);
        if (NULL != conn->parent && NULL != conn->parent->handle) {
            curl_multi_remove_handle(conn->parent->handle, conn->handle);
        }

        curl_easy_cleanup(conn->handle);
    };

    conn->handle = NULL;
}

static void build_curl_request(struct scrobbler_connection *conn)
{
    assert (NULL != conn);

    CURL *handle = conn->handle;

#if defined(LIBCURL_DEBUG) && LIBCURL_DEBUG
    if (_log_level >= log_tracing) {
        curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(handle, CURLOPT_DEBUGFUNCTION, curl_debug);
    }
#endif

    const struct http_request *req = &conn->request;
    struct curl_slist ***req_headers = &conn->headers;

    if (NULL == handle) { return; }
    const enum http_request_types t = req->request_type;

    if (t == http_post) {
        curl_easy_setopt(handle, CURLOPT_POST, 1L);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, req->body);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, (long)req->body_length);
    }

    http_request_print(req, log_tracing2);

    curl_easy_setopt(handle, CURLOPT_PRIVATE, conn);
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, conn->error);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, (long)MAX_WAIT_SECONDS);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_CURLU, req->url);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0L);
    const size_t headers_count = arrlen(req->headers);
    if (headers_count > 0) {
        struct curl_slist *headers = NULL;

        for (size_t i = 0; i < headers_count; i++) {
            struct http_header *header = req->headers[i];
            char full_header[MAX_URL_LENGTH] = {0};
            snprintf(full_header, MAX_URL_LENGTH, "%s: %s", header->name, header->value);

            headers = curl_slist_append(headers, full_header);
        }
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
        arrput(*req_headers, headers);
    }

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, http_response_write_body);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, conn);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, http_response_write_headers);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, conn);
}

static void curl_handler_init(struct scrobbler *s)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    s->handle = curl_multi_init();

    curl_multi_setopt(s->handle, CURLMOPT_SOCKETFUNCTION, curl_request_has_data);
    curl_multi_setopt(s->handle, CURLMOPT_SOCKETDATA, s);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERFUNCTION, curl_request_wait_timeout);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERDATA, s);
}

static void curl_handler_cleanup(struct scrobbler *s)
{
    curl_multi_cleanup(s->handle);
    s->handle = NULL;

    curl_global_cleanup();
}

#endif // MPRIS_SCROBBLER_CURL_H
