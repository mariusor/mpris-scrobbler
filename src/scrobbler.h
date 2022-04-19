/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SCROBBLER_H
#define MPRIS_SCROBBLER_SCROBBLER_H

#include <assert.h>
#include <curl/curl.h>
#include "curl.h"

void scrobbler_connection_free (struct scrobbler_connection *conn)
{
    if (NULL == conn) { return; }
    const char *api_label = get_api_type_label(conn->credentials.end_point);
    _trace("scrobbler::connection_free[%s]", api_label);

    if (NULL != conn->headers) {
        int headers_count = arrlen(conn->headers);
        for (int i = headers_count - 1; i >= 0; i--) {
            _trace2("scrobbler::connection_free::curl_headers(%zd::%zd:%p)", i, headers_count, conn->headers[i]);
            curl_slist_free_all(conn->headers[i]);
            arrdel(conn->headers, i);
            conn->headers[i] = NULL;
        }
        assert(arrlen(conn->headers) == 0);
        arrfree(conn->headers);
        conn->headers = NULL;
    }

    if (event_initialized(&conn->ev)) {
        _trace2("scrobbler::connection_free::event[%p]", &conn->ev);
        event_del(&conn->ev);
    }
    if (event_initialized(&conn->retry_event)) {
        _trace2("scrobbler::connection_free::retry_event[%p]", &conn->retry_event);
        event_del(&conn->retry_event);
    }

    if (NULL != conn->request) {
        _trace2("scrobbler::connection_free::request[%p]", conn->request);
        http_request_free(conn->request);
        conn->request = NULL;
    }
    if (NULL != conn->response) {
        _trace2("scrobbler::connection_free:response[%p]", conn->response);
        http_response_free(conn->response);
        conn->response = NULL;
    }
    if (NULL != conn->handle) {
        _trace2("scrobbler::connection_free:curl_easy_handle[%p]", conn->handle);
        if (NULL != conn->handle) {
            curl_multi_remove_handle(conn->parent->handle, conn->handle);
        }
        curl_easy_cleanup(conn->handle);
        conn->handle = NULL;
    }
    _trace2("scrobbler::connection_free:conn[%p]", conn);
    free(conn);
    conn = NULL;
}

struct scrobbler_connection *scrobbler_connection_new(void)
{
    struct scrobbler_connection *s = calloc(1, sizeof(struct scrobbler_connection));
    s->idx = -1;
    return (s);
}

static void event_cb(int, short, void *);
void scrobbler_connection_init(struct scrobbler_connection *connection, struct scrobbler *s, struct api_credentials credentials, int idx)
{
    connection->handle = curl_easy_init();
    connection->response = http_response_new();
    memcpy(&connection->credentials, &credentials, sizeof(credentials));
    connection->idx = idx;
    connection->parent = s;
    memset(&connection->error, '\0', CURL_ERROR_SIZE);
    _trace("scrobbler::connection_init[%s][%p]:curl_easy_handle(%p)", get_api_type_label(credentials.end_point), connection, connection->handle);

}

static void scrobbler_connections_clean(struct scrobbler *s)
{
    if (s->connections_length == 0) { return; }

    for (int i = s->connections_length - 1; i >= 0; i--) {
        struct scrobbler_connection *conn = s->connections[i];
        if (NULL == conn) {
            continue;
        }

        scrobbler_connection_free(conn);
        s->connections[i] = NULL;
        s->connections_length--;
    }
    _trace2("scrobbler::connection_clean: new len %zd", s->connections_length);
}

static void scrobbler_connection_del(struct scrobbler *s, int idx)
{
    assert(idx != -1);
    if (NULL == s->connections[idx]) {
        return;
    }

    _trace2("scrobbler::connection_del: remove %zd out of %zd: %p", idx, s->connections_length, s->connections[idx]);
    struct scrobbler_connection *conn = s->connections[idx];

    scrobbler_connection_free(conn);

    for (int i = idx + 1; i < s->connections_length; i++) {
        struct scrobbler_connection *to_move = s->connections[i];
        if (NULL == to_move) {
            continue;
        }
        to_move->idx = i-1;
        _trace2("scrobbler::connection_del: move %zd to %zd: %p", i, to_move->idx, to_move);
        s->connections[i-1] = to_move;
    }
    s->connections_length--;
    _trace2("scrobbler::connection_del: new len %zd", s->connections_length);
}

static void scrobbler_clean(struct scrobbler *s)
{
    if (NULL == s) { return; }

    _trace("scrobbler::clean[%p]", s);

    scrobbler_connections_clean(s);
    memset(s->connections, 0x0, sizeof(s->connections));

    if(evtimer_initialized(&s->timer_event) && evtimer_pending(&s->timer_event, NULL)) {
        _trace2("curl::multi_timer_remove(%p)", &s->timer_event);
        evtimer_del(&s->timer_event);
    }
}

static struct scrobbler_connection *scrobbler_connection_get(struct scrobbler *s, CURL *e)
{
    struct scrobbler_connection *conn = NULL;
    for (int i = s->connections_length - 1; i >= 0; i--) {
        conn = s->connections[i];
        if (NULL == conn) {
            _warn("curl::invalid_connection_handle:idx[%d] total[%d]", i, s->connections_length);
            continue;
        }
        if (conn->handle == e) {
            _trace2("curl::found_easy_handle:idx[%d]: %p e[%p]", i, conn, conn->handle);
            break;
        }
        //_trace2("curl::searching_easy_handle:idx[%d] e[%p:%p]", i, conn, e, conn->handle);
    }
    return conn;
}

void scrobbler_init(struct scrobbler *s, struct configuration *config, struct event_base *evbase)
{
    s->credentials = config->credentials;
    s->handle = curl_multi_init();

    /* setup the generic multi interface options we want */
    curl_multi_setopt(s->handle, CURLMOPT_SOCKETFUNCTION, curl_request_has_data);
    curl_multi_setopt(s->handle, CURLMOPT_SOCKETDATA, s);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERFUNCTION, curl_request_wait_timeout);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERDATA, s);

    long max_conn_count = 2.0 * arrlen(s->credentials);
    curl_multi_setopt(s->handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, max_conn_count);

    s->evbase = evbase;

    evtimer_assign(&s->timer_event, s->evbase, timer_cb, s);
    _trace2("curl::multi_timer_add(%p:%p)", s->handle, &s->timer_event);
    s->connections_length = 0;
}

typedef struct http_request*(*request_builder_t)(const struct scrobble*[], const int, const struct api_credentials*, CURL*);

void api_request_do(struct scrobbler *s, const struct scrobble *tracks[], const int track_count, request_builder_t build_request)
{
    if (NULL == s) { return; }
    if (NULL == s->credentials) { return; }

    int credentials_count = arrlen(s->credentials);

    for (int i = 0; i < credentials_count; i++) {
        struct api_credentials *cur = s->credentials[i];
        if (!credentials_valid(cur) ) {
            if (cur->enabled) { _warn("scrobbler::invalid_service[%s]", get_api_type_label(cur->end_point)); }
            continue;
        }

        struct scrobbler_connection *conn = scrobbler_connection_new();
        scrobbler_connection_init(conn, s, *cur, s->connections_length);
        conn->request = build_request(tracks, track_count, cur, conn->handle);
        s->connections[conn->idx] = conn;
        s->connections_length++;

        build_curl_request(conn);

        curl_multi_add_handle(s->handle, conn->handle);
    }
}

#endif // MPRIS_SCROBBLER_SCROBBLER_H
