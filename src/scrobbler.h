/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SCROBBLER_H
#define MPRIS_SCROBBLER_SCROBBLER_H

#include <assert.h>
#include <curl/curl.h>
#include "curl.h"

static bool connection_was_fulfilled(const struct scrobbler_connection *conn)
{
    if (NULL == conn) { return false; }

    const time_t now = time(NULL);
    const double elapsed_seconds = difftime(now, conn->request.time);
    // NOTE(marius): either a CURL error has happened, or the response was returned, or the wait seconds have been exceeded.
    const bool fulfilled = (conn->response.code > 0 && conn->response.code < 600) || elapsed_seconds > MAX_WAIT_SECONDS ||
        strlen(conn->error) > 0 || strlen(conn->response.body) > 0;
    return fulfilled;
}

static void scrobbler_connection_free (struct scrobbler_connection *conn, const bool force)
{
    if (NULL == conn) { return; }
    if (!connection_was_fulfilled(conn)) {
        return;
    }
    if (!force && !connection_was_fulfilled(conn)) { return; }

    const char *api_label = get_api_type_label(conn->credentials.end_point);
    _trace("scrobbler::connection_free[%s]", api_label);

    if (NULL != conn->headers) {
        const size_t headers_count = arrlen(conn->headers);
        for (int i = (int)headers_count - 1; i >= 0; i--) {
            _trace2("scrobbler::connection_free::curl_headers(%zd::%zd:%p)", i, headers_count, conn->headers[i]);
            curl_slist_free_all(conn->headers[i]);
            arrdel(conn->headers, (size_t)i);
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
#ifdef RETRY_ENABLED
    if (event_initialized(&conn->retry_event)) {
        _trace2("scrobbler::connection_free::retry_event[%p]", &conn->retry_event);
        event_del(&conn->retry_event);
    }
#endif

    _trace2("scrobbler::connection_clean::request[%p]", conn->request);
    http_request_clean(&conn->request);
    _trace2("scrobbler::connection_clean:response[%p]", conn->response);
    http_response_clean(&conn->response);
    if (NULL != conn->handle) {
        _trace2("scrobbler::connection_free:curl_easy_handle[%p]", conn->handle);
        if (NULL != conn->parent && NULL != conn->parent->handle) {
            curl_multi_remove_handle(conn->parent->handle, conn->handle);
        }
        curl_easy_cleanup(conn->handle);
        conn->handle = NULL;
    }
    _trace2("scrobbler::connection_free:conn[%p]", conn);
    free(conn);
    conn = NULL;
}

static struct scrobbler_connection *scrobbler_connection_new(void)
{
    struct scrobbler_connection *s = calloc(1, sizeof(struct scrobbler_connection));
    s->idx = -1;
    return (s);
}

static void scrobbler_connection_init(struct scrobbler_connection *connection, struct scrobbler *s, const struct api_credentials credentials, const int idx)
{
    connection->handle = curl_easy_init();
    http_request_init(&connection->request);
    http_response_init(&connection->response);
    memcpy(&connection->credentials, &credentials, sizeof(credentials));
    connection->idx = idx;
    connection->parent = s;
    memset(&connection->error, '\0', CURL_ERROR_SIZE);
    _trace("scrobbler::connection_init[%s][%p]:curl_easy_handle(%p)", get_api_type_label(credentials.end_point), connection, connection->handle);
}

static void scrobbler_connections_clean(struct scrobble_connections *connections, const bool force)
{
    if (force) {
        _trace("scrobbler::connections_clean[%p]: %d", connections, connections->length);
    }
    size_t cleaned = 0;
    size_t skipped = 0;
    for (int i = 0; i < MAX_QUEUE_LENGTH; i++) {
        struct scrobbler_connection *conn = connections->entries[i];
        if (NULL == conn) {
            continue;
        }
        if (!connection_was_fulfilled(conn) && !force) {
            skipped++;
            continue;
        }
        scrobbler_connection_free(conn, force);
        connections->entries[i] = NULL;
        connections->length--;
        cleaned++;
    }
    if (cleaned > 0) {
        _trace("scrobbler::connections_freed: %zu, skipped %zu", cleaned, skipped);
    }
}

static bool scrobbler_queue_is_empty(const struct scrobble_queue *queue)
{
    return (NULL == queue || queue->length == 0);
}

bool configuration_folder_create(const char *);
bool configuration_folder_exists(const char *);
static bool queue_persist_to_file(const struct scrobble_queue *to_persist, const char* path)
{
    bool status = false;

    if (NULL == to_persist || NULL == path) { return status; }
    if (to_persist->length > 0) { return status; }

    char *file_path = (char *)path;
    char *folder_path = dirname(file_path);
    if (!configuration_folder_exists(folder_path) && !configuration_folder_create(folder_path)) {
        _error("main::cache: unable to create cache folder %s", folder_path);
        goto _exit;
    }

    _debug("saving::queue[%u]: %s", to_persist->length, path);
    FILE *file = fopen(path, "w+");
    if (NULL == file) {
        _warn("saving::queue:failed: %s", path);
        goto _exit;
    }
    const size_t wrote = fwrite(to_persist, sizeof(to_persist), 1, file);
    status = wrote == sizeof(to_persist);
    if (!status) {
        _warn("saving::queue:unable to save full file %zu vs. %zu", wrote, sizeof(to_persist));
    }

    fclose(file);

_exit:
    return status;
}

static bool queue_append(struct scrobble_queue *, const struct scrobble *);
static bool scrobbler_persist_queue(const struct scrobbler *scrobbler)
{
    if (NULL == scrobbler || scrobbler_queue_is_empty(&scrobbler->queue)) {
        return false;
    }

    return queue_persist_to_file(&scrobbler->queue, scrobbler->conf->cache_path);
}

static void scrobbler_clean(struct scrobbler *s)
{
    if (NULL == s) { return; }

    scrobbler_persist_queue(s);

    _trace("scrobbler::clean[%p]", s);

    scrobbler_connections_clean(&s->connections, true);

    if(evtimer_initialized(&s->timer_event) && evtimer_pending(&s->timer_event, NULL)) {
        _trace2("curl::multi_timer_remove(%p)", &s->timer_event);
        evtimer_del(&s->timer_event);
    }

    curl_multi_cleanup(s->handle);
    curl_global_cleanup();
}

static struct scrobbler_connection *scrobbler_connection_get(const struct scrobble_connections *connections, const CURL *e)
{
    struct scrobbler_connection *conn = NULL;
    for (int i = 0; i < MAX_QUEUE_LENGTH; i++) {
        conn = connections->entries[i];
        if (NULL == conn) {
            continue;
        }
        if (conn->handle == e) {
            _trace2("curl::found_easy_handle:idx[%d]: %p e[%p]", i, conn, conn->handle);
            break;
        }
    }
    return conn;
}

static void scrobbler_init(struct scrobbler *s, struct configuration *config, struct event_base *evbase)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    s->conf = config;
    s->handle = curl_multi_init();

    s->evbase = evbase;

    evtimer_assign(&s->timer_event, s->evbase, timer_cb, s);
    _trace2("curl::multi_timer_add(%p:%p)", s->handle, &s->timer_event);

    curl_multi_setopt(s->handle, CURLMOPT_SOCKETFUNCTION, curl_request_has_data);
    curl_multi_setopt(s->handle, CURLMOPT_SOCKETDATA, s);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERFUNCTION, curl_request_wait_timeout);
    curl_multi_setopt(s->handle, CURLMOPT_TIMERDATA, s);
    curl_multi_setopt(s->handle, CURLMOPT_MAX_TOTAL_CONNECTIONS, MAX_CREDENTIALS*2L);
    curl_multi_setopt(s->handle, CURLMOPT_MAX_HOST_CONNECTIONS, 2L);

    s->connections.length = 0;
}

typedef void(*request_builder_t)(struct http_request*, const struct scrobble*[MAX_QUEUE_LENGTH], const unsigned, const struct api_credentials*, CURL*);
typedef bool(*request_validation_t)(const struct scrobble*, const struct api_credentials*);

static bool scrobble_is_valid(const struct scrobble *m, const struct api_credentials *cur)
{
    if (NULL == m) {
        return false;
    }

    switch (cur->end_point) {
        case api_listenbrainz:
            return listenbrainz_scrobble_is_valid(m);
        case api_librefm:
        case api_lastfm:
        case api_unknown:
        default:
            return audioscrobbler_scrobble_is_valid(m);
    }
}

static void api_request_do(struct scrobbler *s, const struct scrobble *tracks[], const unsigned track_count, const request_validation_t validate_request, const request_builder_t build_request)
{
    if (NULL == s) { return; }
    if (NULL == s->conf || 0 == s->conf->credentials_count) { return; }

    const size_t credentials_count = s->conf->credentials_count;

    for (size_t i = 0; i < credentials_count; i++) {
        const struct api_credentials *cur = &s->conf->credentials[i];
        if (!credentials_valid(cur)) {
            if (cur->enabled) {
                _warn("scrobbler::invalid_service[%s]", get_api_type_label(cur->end_point));
            }
            continue;
        }
        const struct scrobble *current_api_tracks[MAX_QUEUE_LENGTH] = {0};
        unsigned current_api_track_count = 0;
        for (size_t ti = 0; ti < track_count; ti++) {
            const struct scrobble *track = tracks[ti];
            if (validate_request(track, cur)) {
                current_api_tracks[current_api_track_count] = track;
                current_api_track_count++;
            }
        }
        if (current_api_track_count == 0) {
            _warn("scrobbler::invalid_now_playing[%s]: no valid tracks", get_api_type_label(cur->end_point));
            continue;
        }

        struct scrobbler_connection *conn = scrobbler_connection_new();
        scrobbler_connection_init(conn, s, *cur, s->connections.length);
        build_request(&conn->request, current_api_tracks, current_api_track_count, cur, conn->handle);
        s->connections.entries[conn->idx] = conn;
        s->connections.length++;
        _trace("scrobbler::new_connection[%s]: connections: %zu ", get_api_type_label(cur->end_point), s->connections.length);

        assert(s->connections.length < MAX_QUEUE_LENGTH);

        build_curl_request(conn);

        curl_multi_add_handle(s->handle, conn->handle);
    }
}

#endif // MPRIS_SCROBBLER_SCROBBLER_H
