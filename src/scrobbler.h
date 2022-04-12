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
    _trace("scrobbler::connection_free[%zd:%p]: %s", conn->idx, conn, get_api_type_label(conn->credentials->end_point));

    arrdel(conn->parent->connections, conn->idx);
    if (NULL != conn->headers) {
        int headers_count = arrlen(conn->headers);
        for (int i = headers_count - 1; i >= 0; i--) {
            _trace2("scrobbler::connection_free[%zd]::curl_headers(%zd::%zd:%p)", conn->idx, i, headers_count, conn->headers[i]);
            curl_slist_free_all(conn->headers[i]);
            (void)arrpop(conn->headers);
            conn->headers[i] = NULL;
        }
        assert(arrlen(conn->headers) == 0);
        arrfree(conn->headers);
        conn->headers = NULL;
    }

    _trace2("scrobbler::connection_free[%zd]::conn:(%p)", conn->idx, conn);
    if (event_initialized(&conn->ev)) {
        event_del(&conn->ev);
    }

    if (NULL != conn->request) {
        _trace2("scrobbler::connection_free[%zd]::request(%p)", conn->idx, conn->request);
        http_request_free(conn->request);
        conn->request = NULL;
    }
    if (NULL != conn->response) {
        _trace2("scrobbler::connection_free[%zd]::response(%p)", conn->idx, conn->response);
        http_response_free(conn->response);
        conn->response = NULL;
    }
    if (NULL != conn->handle) {
        _trace2("scrobbler::connection_free[%zd]::curl_easy_handle(%p): parent: %p", conn->idx, conn->handle, conn->parent->handle);
        curl_multi_remove_handle(conn->parent->handle, conn->handle);
        curl_easy_cleanup(conn->handle);
        conn->handle = NULL;
    }
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
void scrobbler_connection_init(struct scrobbler_connection *connection, struct scrobbler *s, struct api_credentials *credentials, int idx)
{
    assert (s);

    connection->handle = curl_easy_init();
    connection->response = http_response_new();
    connection->credentials = credentials;
    connection->idx = idx;
    memset(&connection->error, '\0', CURL_ERROR_SIZE);
    connection->parent = s;
    _trace("scrobbler::connection_init[%s:%d:%p]:curl_easy_handle(%p)", get_api_type_label(credentials->end_point), idx, connection, connection->handle);

}

static void scrobbler_clean(struct scrobbler *s)
{
    if (NULL == s) { return; }

    _trace("scrobbler::clean[%p]", s);

    int conn_count = arrlen(s->connections);
    if (conn_count > 0) {
        for (int i = conn_count - 1; i >= 0;  i--) {
            if (NULL == s->connections[i]) {
                continue;
            }
            scrobbler_connection_free(s->connections[i]);
            s->connections[i] = NULL;
        }
    }
    assert(arrlen(s->connections) == 0);
    arrfree(s->connections);
    s->connections = NULL;

    if(evtimer_initialized(&s->timer_event) && evtimer_pending(&s->timer_event, NULL)) {
        _trace2("curl::multi_timer_remove(%p)", &s->timer_event);
        evtimer_del(&s->timer_event);
    }
}

static struct scrobbler_connection *scrobbler_connection_get(struct scrobbler *s, CURL *e, int *idx)
{
    struct scrobbler_connection *conn = NULL;
    int conn_count = arrlen(s->connections);
    for (int i = conn_count - 1; i >= 0; i--) {
        conn = s->connections[i];
        if (NULL == conn) {
            _warn("curl::invalid_connection_handle:idx[%d] total[%d]", i, conn_count);
            continue;
        }
        if (conn->handle == e) {
            *idx = conn->idx;
            _trace2("curl::found_easy_handle:idx[%d]: %p e[%p]", i, conn, conn->handle);
            break;
        }
        _trace2("curl::searching_easy_handle:idx[%d] e[%p:%p]", i, conn, e, conn->handle);
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

    //curl_multi_setopt(s->handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, 1L);

    s->connections = NULL;
    s->evbase = evbase;

    evtimer_assign(&s->timer_event, s->evbase, timer_cb, s);
    _trace2("curl::multi_timer_add(%p:%p)", s->handle, &s->timer_event);
}

typedef struct http_request*(*request_builder_t)(const struct scrobble*[], const int, const struct api_credentials*, CURL*);

void api_request_do(struct scrobbler *s, const struct scrobble *tracks[], const int track_count, request_builder_t build_request)
{
    if (NULL == s) { return; }
    if (NULL == s->credentials) { return; }

    int credentials_count = arrlen(s->credentials);
    int enabled_credentials_index = 0;

    for (int i = 0; i < credentials_count; i++) {
        struct api_credentials *cur = s->credentials[i];
        if (!credentials_valid(cur) ) {
            if (cur->enabled) { _warn("scrobbler::invalid_service[%s]", get_api_type_label(cur->end_point)); }
            continue;
        }

        struct scrobbler_connection *conn = scrobbler_connection_new();
        scrobbler_connection_init(conn, s, cur, enabled_credentials_index);
        conn->request = build_request(tracks, track_count, cur, conn->handle);

        arrput(s->connections, conn);

        build_curl_request(conn);

        curl_easy_setopt(conn->handle, CURLOPT_PRIVATE, conn);
        curl_easy_setopt(conn->handle, CURLOPT_ERRORBUFFER, conn->error);
        //curl_easy_setopt(conn->handle, CURLOPT_TIMEOUT_MS, 2000L);
        //curl_easy_setopt(conn->handle, CURLOPT_MAXCONNECTS, 1L);
        curl_easy_setopt(conn->handle, CURLOPT_FRESH_CONNECT, 1L);
        curl_easy_setopt(conn->handle, CURLOPT_FORBID_REUSE, 1L);

        curl_multi_add_handle(s->handle, conn->handle);
        enabled_credentials_index++;
    }
}

#endif // MPRIS_SCROBBLER_SCROBBLER_H
