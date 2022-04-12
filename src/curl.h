/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_CURL_H
#define MPRIS_SCROBBLER_CURL_H

#include <curl/curl.h>

//void scrobbler_connection_free(struct scrobbler_connection*);
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
    _trace("curl::check_multi_info[%p]: remaining %d", s, s->still_running);

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

        _info(" api::submitted_to[%s:%d]: %s", get_api_type_label(conn->credentials->end_point), conn->idx, ((conn->response->code == 200) ? "ok" : "nok"));

        //scrobbler_connection_free(conn);
    }
}

/*
 * Based on https://curl.se/libcurl/c/hiperfifo.html
 * Called by libevent when our timeout expires
 */
static void timer_cb(int fd, short kind, void *data)
{
    assert(data);

    struct scrobbler *s = data;
    CURLMcode rc = curl_multi_socket_action(s->handle, CURL_SOCKET_TIMEOUT, 0, &s->still_running);
    if (rc != CURLM_OK) {
        _warn("curl::multi_socket_activation:error: %s", curl_multi_strerror(rc));
        return;
    }
    _trace2("curl::multi_socket_activation[%p:%d]: still_running: %d", s, fd, s->still_running);
    check_multi_info(s);
}

static void scrobbler_clean(struct scrobbler*);
/* Called by libevent when we get action on a multi socket */
static void event_cb(int fd, short kind, void *data)
{
    assert(data);
    struct scrobbler *s = (struct scrobbler*)data;

    int action = ((kind & EV_READ) ? CURL_CSELECT_IN : 0) |
        ((kind & EV_WRITE) ? CURL_CSELECT_OUT : 0);
    _trace2("curl::event_cb(%p:%p:%zd:%zd): still running: %d", s, s->handle, fd, action, s->still_running);

    assert(s->connections);
    assert(arrlen(s->connections) != 0);

    CURLMcode rc = curl_multi_socket_action(s->handle, fd, action, &s->still_running);
    if (rc != CURLM_OK) {
        _warn("curl::transfer::error: %s", curl_multi_strerror(rc));
        return;
    }

    check_multi_info(s);
    if(s->still_running <= 0) {
        _trace2("curl::transfers::finished_all");
        scrobbler_clean(s);
    }
}

/*
 * Based on https://curl.se/libcurl/c/hiperfifo.html
 * Assign information to a scrobbler_connection structure
 */
static void setsock(struct scrobbler_connection *conn, curl_socket_t sock, CURL *e, int act, struct scrobbler *s)
{
    int kind = ((act & CURL_POLL_IN) ? EV_READ : 0) |
        ((act & CURL_POLL_OUT) ? EV_WRITE : 0) | EV_PERSIST;

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
static void dispatch(int, short, void*);
static struct scrobbler_connection *scrobbler_connection_get(struct scrobbler *, CURL *, int *);
/* CURLMOPT_SOCKETFUNCTION */
static int curl_request_has_data(CURL *e, curl_socket_t sock, int what, void *data, void *conn_data)
{
    if (NULL == data) { return 0; }

    struct scrobbler *s = data;
    struct scrobbler_connection *conn = conn_data;
    _trace("curl::data_callback[%p:%zd]: s: %p, conn: %p", e, sock, data, conn_data);

    int events = 0;

    int idx = -1;
    if (NULL == conn) {
        conn = scrobbler_connection_get(s, e, &idx);
        _trace("curl::data_callback_found_connection[%zd:%p]: conn: %p", idx, e, conn);
    }
    if (NULL == conn) {
        // TODO(marius): I'm not sure what effect this has, but for now it prevents a segfault
        return 0;
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
            _trace("curl::data_callback[%zd:%p]: s=%d, action=%s->%s", idx, e, sock, whatstr[conn->action], whatstr[what]);
        } else {
            _trace2("curl::data_callback[%zd:%p]: s=%d, action=%s", idx, e, sock, whatstr[what]);
        }
        return CURLM_OK;
      break;
    case CURL_POLL_REMOVE:
        if (what == CURL_POLL_REMOVE && sock) {
            _trace2("curl::data_remove[%zd:%p]: action=%s", idx, e, whatstr[what]);
            curl_multi_assign(s->handle, sock, NULL);
            //scrobbler_connection_free(conn);
        }
        break;
    default:
        _trace2("curl::unknown_socket_action[%zd:%p]: action=%s", idx, e, whatstr[what]);
        abort();
    }

    return 0;
}

/* Update the event timer after curl_multi library calls */
static int curl_request_wait_timeout(CURLM *multi, long timeout_ms, struct scrobbler *s)
{
    assert(multi);
    assert(s);

    //double timeout_sec = (double)timeout_ms / (double)1000;
    struct timeval timeout = {
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
    /*
    if(timeout_ms == 0) {
        CURLMcode rc = curl_multi_socket_action(multi, CURL_SOCKET_TIMEOUT, 0, &s->still_running);
        if (rc != CURLM_OK) {
            _warn("curl::multi_timer_activation:failed: %s", curl_multi_strerror(rc));
        }
        return 0;
    }
    */
    if (timeout_ms == -1) {
        evtimer_del(&s->timer_event);
    }
    _trace2("curl::multi_timer_update(%p:%p): %d", multi, s->timer_event, timeout_ms);
    evtimer_add(&s->timer_event, &timeout);

    return 0;
}

size_t http_response_write_body(void *buffer, size_t size, size_t nmemb, void* data)
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

    size_t new_size = size * nitems;

    struct http_header *h = http_header_new();

    http_header_load(buffer, nitems, h);
    if (NULL == h->name  || strlen(h->name) == 0) { goto _err_exit; }

    arrput(res->headers, h);
    return new_size;

_err_exit:
    http_header_free(h);
    return new_size;
}

#if DEBUG
static int curl_debug(CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
{
    /* prevent compiler warning */
    (void)handle;
    (void)size;

    data = grrrs_from_string(data);
    grrrs_trim(data, NULL);
    switch(type) {
    case CURLINFO_TEXT:
        _trace("curl::debug: %s", data);
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

void build_curl_request(struct scrobbler_connection *conn)
{
    assert (NULL != conn);

    CURL *handle = conn->handle;

#if DEBUG
    extern enum log_levels _log_level;
    if (_log_level >= log_tracing) {
        curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(handle, CURLOPT_DEBUGFUNCTION, curl_debug);
    }
#endif

    const struct http_request *req = conn->request;
    struct http_response *resp = conn->response;
    struct curl_slist ***req_headers = &conn->headers;

    if (NULL == handle || NULL == req || NULL == resp) { return; }
    enum http_request_types t = req->request_type;

    if (t == http_post) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, req->body);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, (long)req->body_length);
    }

    char *url = http_request_get_url(req);
    http_request_print(req, log_tracing2);

    curl_easy_setopt(handle, CURLOPT_PRIVATE, conn);
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, conn->error);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, 2000L);
    //curl_easy_setopt(handle, CURLOPT_MAXCONNECTS, 1L);
    curl_easy_setopt(handle, CURLOPT_FRESH_CONNECT, 1L);
    curl_easy_setopt(handle, CURLOPT_FORBID_REUSE, 1L);

    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0L);
    int headers_count = arrlen(req->headers);
    if (headers_count > 0) {
        struct curl_slist *headers = NULL;

        for (int i = 0; i < headers_count; i++) {
            struct http_header *header = req->headers[i];
            char *full_header = get_zero_string(MAX_URL_LENGTH);
            snprintf(full_header, MAX_URL_LENGTH, "%s: %s", header->name, header->value);

            headers = curl_slist_append(headers, full_header);

            string_free(full_header);
        }
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
        arrput(*req_headers, headers);
    }
    string_free(url);

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, http_response_write_body);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, resp);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, http_response_write_headers);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, resp);
}

#endif // MPRIS_SCROBBLER_CURL_H
