/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SCROBBLER_H
#define MPRIS_SCROBBLER_SCROBBLER_H

#include <assert.h>
#include <curl/curl.h>

static void check_multi_info(struct scrobbler*);
void scrobbler_connection_free (struct scrobbler_connection *s)
{
    if (NULL == s) { return; }
    if (NULL != s->handle) {
        _trace2("mem::free::scrobbler[%zd]::curl_easy_handle(%p)", s->idx, s->handle);
        curl_easy_cleanup(s->handle);
        s->handle = NULL;
    }
    if (NULL != s->headers) {
        int headers_count = sb_count(s->headers);
        for (int i = 0 ; i < headers_count; i++) {
            _trace2("mem::free::scrobbler[%zd]::curl_headers(%zd::%zd:%p)", s->idx, i, headers_count, s->headers[i]);
            curl_slist_free_all(s->headers[i]);
            (void)sb_add(s->headers, (-1));
        }
        assert(sb_count(s->headers) == 0);
        sb_free(s->headers);
        s->headers = NULL;
    }
    if (NULL != s->ev) {
        _trace2("mem::free::scrobbler[%zd]::conn:(%p)", s->idx, s->ev);
        event_del(s->ev);
        free(s->ev);
        s->ev = NULL;
    }
    if (NULL != s->request) {
        _trace2("mem::free::scrobbler[%zd]::request(%p)", s->idx, s->request);
        http_request_free(s->request);
        s->request = NULL;
    }
    if (NULL != s->response) {
        _trace2("mem::free::scrobbler[%zd]::response(%p)", s->idx, s->response);
        http_response_free(s->response);
        s->response = NULL;
    }
    free(s);
}

struct scrobbler_connection *scrobbler_connection_new(void)
{
    struct scrobbler_connection *s = calloc(1, sizeof(struct scrobbler_connection));
    s->idx = -1;
    return (s);
}

static void event_cb(int, short, void *);
void scrobbler_connection_init(struct scrobbler_connection *connection, struct scrobbler *s, struct api_credentials *credentials, int idx)
{
    connection->handle = curl_easy_init();
    connection->response = http_response_new();
    connection->credentials = credentials;
    connection->ev = calloc(1, sizeof(struct event));
    connection->idx = idx;
    connection->error[0] = '\0';
    connection->parent = s;

    if (NULL != s) {
        event_assign(connection->ev, s->evbase, connection->sockfd, 0, event_cb, s);
    }
}

static void scrobbler_clean(struct scrobbler *s)
{
    if (NULL == s) { return; }

    int con_count = sb_count(s->connections);
    if (con_count > 0) {
        for (int i = 0; i < con_count; i++) {
            scrobbler_connection_free(s->connections[i]);
            s->connections[i] = NULL;
            (void)sb_add(s->connections, (-1));
        }
    }
    assert(sb_count(s->connections) == 0);
    sb_free(s->connections);
    s->connections = NULL;

#if 0
    if(evtimer_pending(s->timer_event, NULL)) {
        _trace2("curl::multi_timer_remove(%p)", s->timer_event);
        evtimer_del(s->timer_event);
    }
#endif
}

/* Check for completed transfers, and remove their easy handles */
static void check_multi_info(struct scrobbler *g)
{
    char *eff_url;
    int msgs_left;
    struct scrobbler_connection *conn;
    CURL *easy;
    CURLMsg *msg;

    while((msg = curl_multi_info_read(g->handle, &msgs_left))) {
        if(msg->msg == CURLMSG_DONE) {
            easy = msg->easy_handle;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);

            curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &conn->response->code);

            if (conn->action == CURL_POLL_REMOVE) {
                if (strlen(conn->error) != 0) {
                    _warn("curl::transfer::done[%zd]: %s => %s", conn->idx, eff_url, conn->error);
                } else {
                    _trace("curl::transfer::done[%zd]: %s", conn->idx, eff_url);
                }

                http_response_print(conn->response, log_tracing2);

                _info(" api::submitted_to[%s:%d:%p]: %s", get_api_type_label(conn->credentials->end_point), conn->idx, conn, ((conn->response->code == 200) ? "ok" : "nok"));
            }
        }
    }
}

/* Assign information to a scrobbler_connection structure */
static void setsock(struct scrobbler_connection *conn, curl_socket_t sock, CURL *e, int act, struct scrobbler *g)
{
    int kind = ((act & CURL_POLL_IN) ? EV_READ : 0) | ((act & CURL_POLL_OUT) ? EV_WRITE : 0) | EV_PERSIST;

    if (conn->handle != e) {
        _error("mismatched curl handle %p %p", conn->handle, e);
        return;
    }

    conn->sockfd = sock;
    conn->action = act;

    event_del(conn->ev);
    event_assign(conn->ev, g->evbase, conn->sockfd, kind, event_cb, g);
    event_add(conn->ev, NULL);

}

/* CURLMOPT_SOCKETFUNCTION */
static int scrobbler_data(CURL *e, curl_socket_t sock, int what, void *data, void *conn_data)
{
    if (NULL == data) { return 0; }
    struct scrobbler *s = (struct scrobbler*)data;
    struct scrobbler_connection *conn = (struct scrobbler_connection*)conn_data;

    int idx = -1;
    int conn_count = sb_count(s->connections);
    for (int i = 0; i < conn_count; i++) {
        conn = s->connections[i];
        if (NULL == conn) {
            _error("curl::invalid_connection_handle:idx[%d] total[%d]", i, conn_count);
            (void)sb_add(s->connections, (-1));
            _debug("curl::removed_invalid_connection_handle:idx[%d] total[%d]", i, conn_count);
            continue;
        }

        if (conn->handle == e) {
            idx = conn->idx;
            break;
        }
        conn = NULL;
    }
    if (NULL != conn) {
        const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE" };
        setsock(conn, sock, e, what, s);
        if (conn->action != what) {
            _trace("curl::data_callback[%zd:%p]: s=%d, action=%s->%s", idx, e, sock, whatstr[conn->action], whatstr[what]);
        } else {
            _trace2("curl::data_callback[%zd:%p]: s=%d, action=%s", idx, e, sock, whatstr[what]);
        }
    }
    return 0;
}

/* Update the event timer after curl_multi library calls */
static int scrobbler_waiting(CURLM *multi, long timeout_ms, struct scrobbler *s)
{
    struct timeval timeout = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000,
    };

    /**
     * if timeout_ms is  0, call curl_multi_socket_action() at once!
     * if timeout_ms is -1, just delete the timer
     *
     * for all other values of timeout_ms, this should set or *update*
     * the timer to the new value
     */
    if(timeout_ms == 0) {
        CURLMcode rc = curl_multi_socket_action(multi, CURL_SOCKET_TIMEOUT, 0, &s->still_running);
        if (rc != CURLM_OK) {
            _warn("curl::multi_timer_activation:failed: %s", curl_easy_strerror(rc));
        }
    } else if(timeout_ms == -1) {
        _trace2("curl::multi_timer_remove(%p)", s->timer_event);
        evtimer_del(s->timer_event);
    } else {
        _trace2("curl::multi_timer_add(%p): %zd.%zds", s->timer_event, timeout.tv_sec, timeout.tv_usec);
        evtimer_add(s->timer_event, &timeout);
    }
    return 0;
}

/* Called by libevent when our timeout expires */
static void timer_cb(int fd, short kind, void *data)
{
    struct scrobbler *s = (struct scrobbler *)data;
    CURLMcode rc = curl_multi_socket_action(s->handle, CURL_SOCKET_TIMEOUT, 0, &s->still_running);
    if (rc != CURLM_OK) {
        _warn("curl::multi_socket_activation:error[%d:%d]: %s", fd, kind, curl_easy_strerror(rc));
    }
    //check_multi_info(s);
}

struct scrobbler *scrobbler_new(void)
{
    struct scrobbler *result = calloc(1, sizeof(struct scrobbler));

    return (result);
}

void scrobbler_init(struct scrobbler *s, struct configuration *config, struct events *events)
{
    s->credentials = config->credentials;
    s->handle = curl_multi_init();

    /* setup the generic multi interface options we want */
    curl_multi_setopt(s->handle, CURLMOPT_SOCKETFUNCTION, scrobbler_data);
    curl_multi_setopt(s->handle, CURLMOPT_SOCKETDATA, s);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERFUNCTION, scrobbler_waiting);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERDATA, s);

    s->connections = NULL;
    s->evbase = events->base;
    s->timer_event = events->curl_timer;

    _trace("scrobbler::assigning:timer_event(%p)", s->timer_event);
    evtimer_assign(s->timer_event, events->base, timer_cb, s);
}

void scrobbler_free(struct scrobbler *s)
{
    if (NULL == s) { return; }

    scrobbler_clean(s);

    if (NULL != s->handle) {
        _trace2("mem::free::scrobbler::curl_multi(%p)", s->handle);
        curl_multi_cleanup(s->handle);
        s->handle = NULL;
    }
    free(s);
}

/* Called by libevent when we get action on a multi socket */
static void event_cb(int fd, short kind, void *data)
{
    struct scrobbler *s = (struct scrobbler*)data;

    int action = ((kind & EV_READ) ? CURL_CSELECT_IN : 0) | ((kind & EV_WRITE) ? CURL_CSELECT_OUT : 0);
    //_trace2("curl::event_cb(%p:%p:%zd:%zd): still running: %s", s, s->handle, fd, action, (s->still_running ? "yes" : "no"));

    int con_count = sb_count(s->connections);
    assert(s->connections);
    assert(con_count != 0);

    CURLMcode rc = curl_multi_socket_action(s->handle, fd, action, &s->still_running);
    if (rc != CURLM_OK) {
        _warn("curl::transfer::error: %s", curl_easy_strerror(rc));
    }

    check_multi_info(s);
    if(s->still_running <= 0) {
        _trace("curl::transfers::finished_all");
        scrobbler_clean(s);
    }
}

void api_request_do(struct scrobbler *s, const struct scrobble *tracks[], struct http_request*(*build_request)(const struct scrobble*[], const struct api_credentials*))
{
    if (NULL == s) { return; }

    if (s->credentials == 0) { return; }

    int credentials_count = sb_count(s->credentials);

    for (int i = 0; i < credentials_count; i++) {
        struct api_credentials *cur = s->credentials[i];
        if (!credentials_valid(cur)) {
            _warn("scrobbler::invalid_service[%s]", get_api_type_label(cur->end_point));
            continue;
        }
        // check if we already have in the connections array one matching the current api
        for (int i = 0; i < sb_count(s->connections); i++) {
            struct scrobbler_connection *existing = s->connections[i];

            if (existing->credentials->end_point == cur->end_point) {
            _warn("scrobbler::api_call_already_queued[%s]", get_api_type_label(cur->end_point));
                continue;
            }
        }

        struct scrobbler_connection *conn = scrobbler_connection_new();
        scrobbler_connection_init(conn, s, cur, i);
        conn->request = build_request(tracks, cur);

        sb_push(s->connections, conn);

        build_curl_request(conn->handle, conn->request, conn->response, &conn->headers);

        curl_easy_setopt(conn->handle, CURLOPT_PRIVATE, conn);
        curl_easy_setopt(conn->handle, CURLOPT_ERRORBUFFER, conn->error);

        curl_multi_add_handle(s->handle, conn->handle);
    }
}

#endif // MPRIS_SCROBBLER_SCROBBLER_H
