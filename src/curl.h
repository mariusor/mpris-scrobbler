/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_CURL_H
#define MPRIS_SCROBBLER_CURL_H

#include <curl/curl.h>

#ifdef RETRY_ENABLE

#define MAX_RETRIES 5

static void retry_cb(int fd, short kind, void *data)
{
    assert(data);

    const struct scrobbler_connection *conn = data;
    curl_multi_add_handle(conn->parent->handle, conn->handle);
}

static void connection_retry(struct scrobbler_connection *conn)
{
    const struct timeval retry_timeout = { .tv_sec = 3 + conn->retries, .tv_usec = 0, };
    if (NULL != conn->response) {
        http_response_clean(conn->response);
    }

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
    const int code = (int)conn->response->code;
    if (code >= 400 && code < 500) {
        // NOTE(marius): 4XX errors mean that there's something wrong with our request
        // so we shouldn't retry.
        return false;
    }
    return conn->retries < MAX_RETRIES;
}
#endif

/*
 * Based on https://curl.se/libcurl/c/hiperfifo.html
 * Check for completed transfers, and remove their easy handles
 */
static void check_multi_info(struct scrobbler *s)
{
    char *eff_url;
    int msgs_left;
    struct scrobbler_connection *conn;
    CURL *easy;
    CURLMsg *msg;
    CURLcode res;
    _trace2("curl::check_multi_info[%p]: remaining %d", s, s->still_running);

    while((msg = curl_multi_info_read(s->handle, &msgs_left))) {
        if(msg->msg != CURLMSG_DONE) {
            continue;
        }
        easy = msg->easy_handle;
        res = msg->data.result;
        curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
        curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &conn->response->code);
        if (strlen(conn->error) != 0) {
            _warn("curl::transfer::done[%zd]: %s => (%d) %s", conn->idx, eff_url, res, conn->error);
        } else {
            _trace("curl::transfer::done[%zd]: %s", conn->idx, eff_url);
        }

        http_response_print(conn->response, log_tracing2);

        const bool success = (conn->response != NULL && conn->response->code == 200);
        _info(" api::submitted_to[%s]: %s", get_api_type_label(conn->credentials.end_point), (success ? "ok" : "nok"));
        if(evtimer_pending(&s->timer_event, NULL)) {
            _trace2("curl::multi_timer_remove(%p)", &s->timer_event);
            evtimer_del(&s->timer_event);
        }
#ifdef RETRY_ENABLE
        if (!success && connection_should_retry(conn)) {
            connection_retry(conn);
            return;
        }
#endif
        if (success) {
            conn->should_free = true;
        }
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
    if (rc != CURLM_OK) {
        _warn("curl::multi_socket_activation:error: %s", curl_multi_strerror(rc));
        return;
    }
    _trace2("curl::multi_socket_activation[%p:%d]: still_running: %d", s, fd, s->still_running);

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

/*
 * Based on https://curl.se/libcurl/c/hiperfifo.html
 * Assign information to a scrobbler_connection structure
 */
static void setsock(struct scrobbler_connection *conn, curl_socket_t sock, CURL *e, int act, struct scrobbler *s)
{
    const short kind = ((act & CURL_POLL_IN) ? EV_READ : 0) | ((act & CURL_POLL_OUT) ? EV_WRITE : 0) | EV_PERSIST;

    if (conn->handle != e) {
        _trace("curl::mismatched_handle %p %p kind %zd", conn->handle, e, kind);
        return;
    }

    conn->sockfd = sock;
    conn->action = act;

    if (event_initialized(&conn->ev)) {
        event_del(&conn->ev);
    }

    curl_multi_assign(s->handle, conn->sockfd, conn);
    evutil_make_socket_nonblocking(conn->sockfd);
    event_assign(&conn->ev, s->evbase, conn->sockfd, kind, event_cb, s);
    event_add(&conn->ev, NULL);
}

const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE" };
static struct scrobbler_connection *scrobbler_connection_get(const struct scrobble_connections*, const CURL*);
/* CURLMOPT_SOCKETFUNCTION */
static int curl_request_has_data(CURL *e, const curl_socket_t sock, const int what, void *data, void *conn_data)
{
    if (NULL == data) { return CURLM_OK; }

    struct scrobbler *s = data;
    int events = 0;

    struct scrobbler_connection *conn = conn_data;
    _trace2("curl::data_callback[%p:%zd]: s: %p, conn: %p", e, sock, data, conn_data);
    if (NULL == conn) {
        conn = scrobbler_connection_get(&s->connections, e);
        if (NULL == conn) {
            const CURLcode status = curl_easy_getinfo(e, CURLINFO_PRIVATE, &conn);
            if (NULL == conn) {
                return status;
            }
        }
    }

    switch(what) {
    case CURL_POLL_IN:
    case CURL_POLL_OUT:
    case CURL_POLL_INOUT:
        // set libevent context for a new connection
        if(what != CURL_POLL_IN) events |= EV_WRITE;
        if(what != CURL_POLL_OUT) events |= EV_READ;

        events |= EV_PERSIST;
        setsock(conn, sock, e, what, s);
        if (conn->action != what) {
            _trace2("curl::data_callback[%zd:%p]: s=%d, action=%s->%s", conn->idx, e, sock, whatstr[conn->action], whatstr[what]);
        } else {
            _trace2("curl::data_callback[%zd:%p]: s=%d, action=%s", conn->idx, e, sock, whatstr[what]);
        }
      break;
    case CURL_POLL_REMOVE:
        if (sock) {
            _trace2("curl::data_remove[%p]: action=%s", e, whatstr[what]);
            curl_multi_assign(s->handle, sock, NULL);
            if (NULL != conn) { conn->should_free = true; }
        }
        break;
    default:
        _trace2("curl::unknown_socket_action[%p]: action=%s", e, whatstr[what]);
        assert(false);
    }

    return CURLM_OK;
}

/* Update the event timer after curl_multi library calls */
static int curl_request_wait_timeout(CURLM *multi, const long timeout_ms, struct scrobbler *s)
{
    assert(multi);
    assert(s);

    const struct timeval timeout = {
        .tv_sec = timeout_ms/1000,
        .tv_usec = (timeout_ms % 1000) * 1000,
    };

    /**
     * if timeout_ms is  0, call curl_multi_socket_action() at once!
     * if timeout_ms is -1, just delete the timer
     *
     * for all other values of timeout_ms, this should set or *update*
     * the timer to the new value
     */
    _trace2("curl::multi_timer_triggered(%p:%p):still_running: %d, timeout: %d", s, &s->timer_event, s->still_running, timeout_ms);
    if (timeout_ms == -1) {
        evtimer_del(&s->timer_event);
        return 0;
    }
    _trace2("curl::multi_timer_update(%p:%p): %d", multi, s->timer_event, timeout_ms);
    evtimer_add(&s->timer_event, &timeout);

    return 0;
}

static size_t http_response_write_body(void *buffer, size_t size, size_t nmemb, void* data)
{
    if (NULL == buffer) { return 0; }
    if (NULL == data) { return 0; }
    if (0 == size) { return 0; }
    if (0 == nmemb) { return 0; }

    struct http_response *res = (struct http_response*)data;

    size_t new_size = size * nmemb;

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

    struct http_response *res = data;

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
static int curl_debug(CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
{
    /* prevent compiler warning */
    (void)handle;
    (void)size;

    data = grrrs_from_string(data);
    grrrs_trim(data, NULL);
    switch(type) {
    case CURLINFO_TEXT:
        _trace2("curl::debug: %s", data);
        break;
    case CURLINFO_HEADER_OUT:
        _trace2("curl::debug: Send header: %s", data);
        break;
    case CURLINFO_DATA_OUT:
        _trace2("curl::debug: Send data: %s", data);
        break;
    case CURLINFO_HEADER_IN:
        _trace2("curl::debug: Recv header: %s", data);
        break;
    case CURLINFO_DATA_IN:
        _trace2("curl::debug: Recv data: %s", data);
        break;
    case CURLINFO_SSL_DATA_OUT:
        _trace2("curl::debug: Send SSL data");
        break;
    case CURLINFO_SSL_DATA_IN:
        _trace2("curl::debug: Recv SSL data");
        break;
    default: /* in case a new one is introduced to shock us */
        break;
    }
    grrrs_free(data);

    return 0;
}
#endif

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

    const struct http_request *req = conn->request;
    struct http_response *resp = conn->response;
    struct curl_slist ***req_headers = &conn->headers;

    if (NULL == handle || NULL == req || NULL == resp) { return; }
    const enum http_request_types t = req->request_type;

    if (t == http_post) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, req->body);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, (long)req->body_length);
    }

    http_request_print(req, log_tracing2);

    curl_easy_setopt(handle, CURLOPT_PRIVATE, conn);
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, conn->error);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, 6000L);

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
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, resp);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, http_response_write_headers);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, resp);
}

#endif // MPRIS_SCROBBLER_CURL_H
