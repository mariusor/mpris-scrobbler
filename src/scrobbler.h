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
        if (NULL != conn->parent && NULL != conn->parent->handle && NULL != conn->handle) {
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
    if (s->connections.length == 0) { return; }

    for (int i = s->connections.length - 1; i >= 0; i--) {
        struct scrobbler_connection *conn = s->connections.entries[i];
        if (NULL == conn) {
            continue;
        }

        scrobbler_connection_free(conn);
        s->connections.entries[i] = NULL;
        s->connections.length--;
    }
    _trace2("scrobbler::connection_clean: new len %zd", s->connections.length);
}

static void scrobbler_connection_del(struct scrobbler *s, const int idx)
{
    assert(idx != -1);
    if (NULL == s->connections.entries[idx]) {
        return;
    }

    _trace2("scrobbler::connection_del: remove %zd out of %zd: %p", idx, s->connections.length, s->connections.entries[idx]);
    struct scrobbler_connection *conn = s->connections.entries[idx];

    scrobbler_connection_free(conn);

    for (int i = idx + 1; i < s->connections.length; i++) {
        struct scrobbler_connection *to_move = s->connections.entries[i];
        if (NULL == to_move) {
            continue;
        }
        to_move->idx = i-1;
        _trace2("scrobbler::connection_del: move %zd to %zd: %p", i, to_move->idx, to_move);
        s->connections.entries[i-1] = to_move;
    }
    s->connections.length--;
    _trace2("scrobbler::connection_del: new len %zd", s->connections.length);
}

bool scrobbler_queue_is_empty(const struct scrobble_queue queue)
{
    return (queue.length == 0);
}

bool configuration_folder_create(const char *);
bool configuration_folder_exists(const char *);
bool queue_persist_to_file(const struct scrobble_queue to_persist, const char* path)
{
    bool status = false;
    if (to_persist.length > 0) {
        return status;
    }

    char *file_path = grrrs_from_string(path);
    char *folder_path = dirname(file_path);
    if (!configuration_folder_exists(folder_path) && !configuration_folder_create(folder_path)) {
        _error("main::cache: unable to create cache folder %s", folder_path);
        goto _exit;
    }

    _debug("saving::queue[%u]: %s", to_persist.length, path);
    FILE *file = fopen(path, "w+");
    if (NULL == file) {
        _warn("saving::queue:failed: %s", path);
        goto _exit;
    }
    const size_t wrote = fwrite(&to_persist, sizeof(to_persist), 1, file);
    status = wrote == sizeof(to_persist);
    if (!status) {
        _warn("saving::queue:unable to save full file %zu vs. %zu", wrote, sizeof(to_persist));
    }

    fclose(file);

_exit:
    grrrs_free(file_path);
    return status;
}

static bool scrobble_is_valid(const struct scrobble *);
bool queue_append(struct scrobble_queue *, const struct scrobble *);
bool scrobbler_persist_queue(const struct scrobbler *scrobbler)
{
    if (scrobbler_queue_is_empty(scrobbler->queue)) {
        return false;
    }

    return queue_persist_to_file(scrobbler->queue, scrobbler->conf->cache_path);
}

static void scrobbler_clean(struct scrobbler *s)
{
    if (NULL == s) { return; }

    scrobbler_persist_queue(s);

    _trace("scrobbler::clean[%p]", s);

    scrobbler_connections_clean(s);
    memset(s->connections.entries, 0x0, sizeof(s->connections.entries));

    if(evtimer_initialized(&s->timer_event) && evtimer_pending(&s->timer_event, NULL)) {
        _trace2("curl::multi_timer_remove(%p)", &s->timer_event);
        evtimer_del(&s->timer_event);
    }
}

static struct scrobbler_connection *scrobbler_connection_get(struct scrobbler *s, CURL *e)
{
    struct scrobbler_connection *conn = NULL;
    for (int i = s->connections.length - 1; i >= 0; i--) {
        conn = s->connections.entries[i];
        if (NULL == conn) {
            _warn("curl::invalid_connection_handle:idx[%d] total[%d]", i, s->connections.length);
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
    s->conf = config;
    s->handle = curl_multi_init();

    /* set up the generic multi interface options we want */
    curl_multi_setopt(s->handle, CURLMOPT_SOCKETFUNCTION, curl_request_has_data);
    curl_multi_setopt(s->handle, CURLMOPT_SOCKETDATA, s);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERFUNCTION, curl_request_wait_timeout);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERDATA, s);

    long max_conn_count = 2.0 * arrlen(s->conf->credentials);
    curl_multi_setopt(s->handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, max_conn_count);

    s->evbase = evbase;

    evtimer_assign(&s->timer_event, s->evbase, timer_cb, s);
    _trace2("curl::multi_timer_add(%p:%p)", s->handle, &s->timer_event);
    s->connections.length = 0;
}

typedef struct http_request*(*request_builder_t)(const struct scrobble*[], const int, const struct api_credentials*, CURL*);

void api_request_do(struct scrobbler *s, const struct scrobble *tracks[], const int track_count, request_builder_t build_request)
{
    if (NULL == s) { return; }
    if (NULL == s->conf || NULL == s->conf->credentials) { return; }

    int credentials_count = arrlen(s->conf->credentials);

    for (int i = 0; i < credentials_count; i++) {
        const struct api_credentials *cur = s->conf->credentials[i];
        if (!credentials_valid(cur)) {
            if (cur->enabled) {
                _warn("scrobbler::invalid_service[%s]", get_api_type_label(cur->end_point));
            }
            continue;
        }

        struct scrobbler_connection *conn = scrobbler_connection_new();
        scrobbler_connection_init(conn, s, *cur, s->connections.length);
        conn->request = build_request(tracks, track_count, cur, conn->handle);
        s->connections.entries[conn->idx] = conn;
        s->connections.length++;

        build_curl_request(conn);

        curl_multi_add_handle(s->handle, conn->handle);
    }
}

#endif // MPRIS_SCROBBLER_SCROBBLER_H
