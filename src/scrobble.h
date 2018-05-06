/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SCROBBLE_H
#define MPRIS_SCROBBLER_SCROBBLE_H

#include <assert.h>
#include <curl/curl.h>
#include <time.h>

#define NOW_PLAYING_DELAY 65 //seconds
#define MIN_TRACK_LENGTH  30 // seconds
#define MPRIS_SPOTIFY_TRACK_ID_PREFIX                          "spotify:track:"

char *get_player_namespace(DBusConnection *);
void get_mpris_properties(DBusConnection*, const char*, struct mpris_properties*, struct mpris_event*);

struct dbus *dbus_connection_init(struct state*);
void state_loaded_properties(struct state*, struct mpris_properties*, const struct mpris_event*);

#if 0
const char *get_api_status_label (api_return_codes code)
{
    switch (code) {
        case unavaliable:
            return "The service is temporarily unavailable, please try again.";
        case invalid_service:
            return "Invalid service - This service does not exist";
        case invalid_method:
            return "Invalid Method - No method with that name in this package";
        case authentication_failed:
            return "Authentication Failed - You do not have permissions to access the service";
        case invalid_format:
            return "Invalid format - This service doesn't exist in that format";
        case invalid_parameters:
            return "Invalid parameters - Your request is missing a required parameter";
        case invalid_resource:
            return "Invalid resource specified";
        case operation_failed:
            return "Operation failed - Something else went wrong";
        case invalid_session_key:
            return "Invalid session key - Please re-authenticate";
        case invalid_apy_key:
            return "Invalid API key - You must be granted a valid key by last.fm";
        case service_offline:
            return "Service Offline - This service is temporarily offline. Try again later.";
        case invalid_signature:
            return "Invalid method signature supplied";
        case temporary_error:
            return "There was a temporary error processing your request. Please try again";
        case suspended_api_key:
            return "Suspended API key - Access for your account has been suspended, please contact Last.fm";
        case rate_limit_exceeded:
            return "Rate limit exceeded - Your IP has made too many requests in a short period";
    }
    return "Unkown";
}
#endif

static void scrobble_init(struct scrobble *s)
{
    if (NULL == s) { return; }
    s->scrobbled = false;
    s->length = 0;
    s->position = 0;
    s->start_time = 0;
    s->track_number = 0;

    s->title = get_zero_string(MAX_PROPERTY_LENGTH);
    s->album = get_zero_string(MAX_PROPERTY_LENGTH);
    s->artist = NULL;

    s->mb_track_id = get_zero_string(MAX_PROPERTY_LENGTH);
    s->mb_artist_id = get_zero_string(MAX_PROPERTY_LENGTH);
    s->mb_album_id = get_zero_string(MAX_PROPERTY_LENGTH);
    s->mb_album_artist_id = get_zero_string(MAX_PROPERTY_LENGTH);
    s->mb_spotify_id = get_zero_string(MAX_PROPERTY_LENGTH);

    _trace2("mem::inited_scrobble(%p)", s);
}

struct scrobble *scrobble_new(void)
{
    struct scrobble *result = malloc(sizeof(struct scrobble));
    scrobble_init(result);

    return result;
}

void scrobble_free(struct scrobble *s)
{
    if (NULL == s) { return; }

    if (NULL != s->title) {
        if (strlen(s->title) > 0) { _trace2("mem::scrobble::free::title(%p): %s", s->title, s->title); }
        free(s->title);
        s->title = NULL;
    }
    if (NULL != s->album) {
        if (strlen(s->album) > 0) { _trace2("mem::scrobble::free::album(%p): %s", s->album, s->album); }
        free(s->album);
        s->album = NULL;
    }
    if (NULL != s->artist) {
        sb_arr_free(&s->artist);
    }
    if (NULL != s->mb_track_id) {
        if (NULL != s->mb_track_id) {
            if (strlen(s->mb_track_id) > 0) { _trace2("mem::scrobble::musicbrainz::free::track_id(%p): %s", s->mb_track_id, s->mb_track_id); }
            free(s->mb_track_id);
            s->mb_track_id = NULL;
        }
        if (NULL != s->mb_album_id) {
            if (strlen(s->mb_album_id) > 0) { _trace2("mem::scrobble::musicbrainz::free::album_id(%p): %s", s->mb_album_id, s->mb_album_id); }
            free(s->mb_album_id);
            s->mb_album_id = NULL;
        }
        if (NULL != s->mb_artist_id) {
            if (strlen(s->mb_artist_id) > 0) { _trace2("mem::scrobble::musicbrainz::free::artist_id(%p): %s", s->mb_artist_id, s->mb_artist_id); }
            free(s->mb_artist_id);
            s->mb_artist_id = NULL;
        }
        if (NULL != s->mb_album_artist_id) {
            if (strlen(s->mb_album_artist_id) > 0) { _trace2("mem::scrobble::musicbrainz::free::album_artist_id(%p): %s", s->mb_album_artist_id, s->mb_album_artist_id); }
            free(s->mb_album_artist_id);
            s->mb_album_artist_id = NULL;
        }
        if (NULL != s->mb_spotify_id) {
            if (strlen(s->mb_spotify_id) > 0) { _trace2("mem::scrobble::musicbrainz::free::spotify_id(%p): %s", s->mb_spotify_id, s->mb_spotify_id); }
            free(s->mb_spotify_id);
            s->mb_spotify_id = NULL;
        }
    }

    _trace2("mem::free::scrobble(%p)", s);
    free(s);
}

void scrobbles_free(struct scrobble *tracks[])
{
    if (NULL == tracks) { return; }

    int track_count = sb_count(tracks);
    if (track_count == 0) { return; }

    for (int i = 0; i < track_count; i++) {
        _trace("scrobbler::freeing_track(%zu:%p) ", i, tracks[i]);
        (void)sb_add(tracks, (-1));
        scrobble_free(tracks[i]);
        tracks[i] = NULL;
    }
    assert(sb_count(tracks) == 0);
    //sb_free(tracks);
}

void scrobbler_connection_free(struct scrobbler_connection*);
static void scrobbler_clean(struct scrobbler *s)
{
    if (NULL == s) { return; }


    int con_count = sb_count(s->connections);
    if (con_count > 0) {
        for (int i = 0; i < con_count; i++) {
            scrobbler_connection_free(s->connections[i]);
            (void)sb_add(s->connections, (-1));
            s->connections[i] = NULL;
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

static void scrobbler_free(struct scrobbler *s)
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

#if 0
static bool scrobbles_remove(struct mpris_properties *queue[], size_t queue_length, size_t pos)
{
    struct mpris_properties *last = queue[pos];
    if (NULL == last) { return false; }
    struct mpris_metadata *d = last->metadata;
    _trace("scrobbler::remove_scrobble(%p//%u) %s//%s//%s", last, pos, d->title, d->artist, d->album);
    if (pos >= queue_length) { return queue_length; }

    mpris_properties_free(last);
    queue[pos] = NULL;

    //player->previous = player->queue[pos-1];
    return true;
}
#endif

static void scrobble_init(struct scrobble*);
static void mpris_player_free(struct mpris_player *player)
{
    if (NULL == player) { return; }

    int queue_count = sb_count(player->queue);
    _trace2("mem::free::player(%p)::queue_length:%u", player, queue_count);
    for (int i = 0; i < queue_count; i++) {
        (void)sb_add(player->queue, (-1));
        scrobble_free(player->queue[i]);
        player->queue[i] = NULL;
    }
    assert(sb_count(player->queue) == 0);
    sb_free(player->queue);
    if (NULL != player->mpris_name) { free(player->mpris_name); }
    if (NULL != player->properties) { mpris_properties_free(player->properties); }
    if (NULL != player->current) { mpris_properties_free(player->current); }
    if (NULL != player->changed) { free(player->changed); }

    free (player);
}

void events_free(struct events *);
void dbus_close(struct state *);
void state_free(struct state *s)
{
    if (NULL != s->dbus) { dbus_close(s); }
    if (NULL != s->events) { events_free(s->events); }
    if (NULL != s->player) { mpris_player_free(s->player); }
    if (NULL != s->scrobbler) { scrobbler_free(s->scrobbler); }

    free(s);
}

static void check_multi_info(struct scrobbler*);
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

struct scrobbler_connection *scrobbler_connection_new()
{
    struct scrobbler_connection *s = calloc(1, sizeof(struct scrobbler_connection));
    s->idx = -1;
    return (s);
}

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

            if (strlen(conn->error) != 0) {
                _warn("curl::transfer::done[%zd]: %s => %s", conn->idx, eff_url, conn->error);
            } else {
                _trace("curl::transfer::done[%zd]: %s", conn->idx, eff_url);
            }

            curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &conn->response->code);

            if (conn->action == CURL_POLL_REMOVE) {
                http_response_print(conn->response, tracing2);
            }

            if (conn->response->code == 200) {
                _info("api::submitted_to[%s]: ok", get_api_type_label(conn->credentials->end_point));
            } else {
                _info("api::submitted_to[%s]: nok", get_api_type_label(conn->credentials->end_point));
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

static void scrobbler_init(struct scrobbler *s, struct configuration *config, struct events *events)
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

static struct mpris_player *mpris_player_new(void)
{
    struct mpris_player *result = calloc(1, sizeof(struct mpris_player));

    return (result);
}

static void mpris_player_init(struct mpris_player *player, DBusConnection *conn)
{
    player->mpris_name = NULL;
    player->changed = calloc(1, sizeof(struct mpris_event));
    player->queue = NULL;

    if (NULL != conn) {
        player->properties = mpris_properties_new();
        _trace("mem::player::inited_properties(%p)", player->properties);
        player->current = mpris_properties_new();
        _trace("mem::player::inited_current(%p)", player->current);

        player->mpris_name = get_player_namespace(conn);
        if (NULL != player->mpris_name) {
            get_mpris_properties(conn, player->mpris_name, player->properties, player->changed);
        }
    }
    _trace("mem::player::inited(%p)", player);
}

struct events *events_new(void);
void events_init(struct events*, struct sighandler_payload*);
bool state_init(struct state *s, struct sighandler_payload *sig_data)
{
    _trace2("mem::initing_state(%p)", s);
    s->scrobbler = scrobbler_new();
    s->player = mpris_player_new();

    s->events = events_new();
    events_init(s->events, sig_data);
    s->dbus = dbus_connection_init(s);
    scrobbler_init(s->scrobbler, sig_data->config, s->events);

    if (NULL == s->dbus) { return false; }
    mpris_player_init(s->player, s->dbus->conn);
    _trace2("mem::inited_state(%p)", s);

    state_loaded_properties(s, s->player->properties, s->player->changed);

    return true;
}

struct state *state_new(void)
{
    struct state *result = malloc(sizeof(struct state));
    return result;
}

static bool scrobble_is_valid(const struct scrobble *s)
{
    if (NULL == s) { return false; }
    if (NULL == s->title) { return false; }
    if (NULL == s->album) { return false; }
    if (sb_count(s->artist) == 0 || NULL == s->artist[0]) { return false; }
#if 0
    _trace("Checking playtime: %u - %u", s->play_time, (s->length / 2));
    _trace("Checking scrobble:\n" \
        "title: %s\n" \
        "artist: %s\n" \
        "album: %s\n" \
        "length: %u\n" \
        "play_time: %u\n", s->title, s->artist, s->album, s->length, s->play_time);
#endif
    return (
        s->length >= MIN_TRACK_LENGTH &&
        //s->play_time >= (s->length / 2) &&
        s->scrobbled == false &&
        strlen(s->title) > 0 &&
        strlen(s->artist[0]) > 0 &&
        strlen(s->album) > 0
        );
}

bool now_playing_is_valid(const struct scrobble *m/*, const time_t current_time, const time_t last_playing_time*/) {
#if 0
    _trace("Checking playtime: %u - %u", m->play_time, (m->length / 2));
    _trace("Checking scrobble:\n" \
        "title: %s\n" \
        "artist: %s\n" \
        "album: %s\n" \
        "length: %u\n" \
        "play_time: %u\n", m->title, m->artist, m->album, m->length, m->play_time);
#endif
    return (
        NULL != m &&
        NULL != m->title && strlen(m->title) > 0 &&
        (
         (sb_count(m->artist) > 0 && NULL != m->artist[0] && strlen(m->artist[0]) > 0) ||
         (NULL != m->album && strlen(m->album) > 0)
        ) &&
        //        last_playing_time > 0 &&
        //        difftime(current_time, last_playing_time) >= LASTFM_NOW_PLAYING_DELAY &&
        m->length > 0
        );
}

#if 0
void scrobble_print(const struct scrobble *s, enum log_levels log) {
    _log(log, "scrobble[%p]: \n" \
        "\ttitle: %s\n" \
        "\tartist: %s\n" \
        "\talbum: %s",
        s->title, s->artist[0], s->album);
}

static void scrobble_zero (struct scrobble *s)
{
    if (NULL == s) { return; }
    if (NULL != s->title) {
        memset(s->title, 0, strlen(s->title));
    }
    if (NULL != s->album) {
        memset(s->album, 0, strlen(s->album));
    }
    if (NULL != s->artist) {
        memset(s->artist, 0, strlen(s->artist));
    }
    if (NULL != s->mb_track_id) {
        memset(s->mb_track_id, 0, strlen(s->mb_track_id));
    }
    if (NULL != s->mb_album_id) {
        memset(s->mb_album_id, 0, strlen(s->mb_album_id));
    }
    if (NULL != s->mb_artist_id) {
        memset(s->mb_artist_id, 0, strlen(s->mb_artist_id));
    }
    if (NULL != s->mb_album_artist_id) {
        memset(s->mb_album_artist_id, 0, strlen(s->mb_album_artist_id));
    }
    if (NULL != s->mb_spotify_id) {
        memset(s->mb_spotify_id, 0, strlen(s->mb_spotify_id));
    }

    s->length = 0;
    s->position = 0;
    s->start_time = 0;
    s->play_time = 0;
    s->track_number = 0;
}
#endif

static void scrobble_copy (struct scrobble *t, const struct scrobble *s)
{
    if (NULL == t) { return; }
    if (NULL == s) { return; }

    assert(s->title);
    assert(s->album);
    assert(s->artist[0]);
    assert(t->album);
    assert(t->title);


    strncpy(t->title, s->title, MAX_PROPERTY_LENGTH);
    strncpy(t->album, s->album, MAX_PROPERTY_LENGTH);

    for (int i = 0; i < sb_count(s->artist); i++) {
        char *elem = get_zero_string(MAX_PROPERTY_LENGTH);
        strncpy(elem, s->artist[i], MAX_PROPERTY_LENGTH);
        sb_push(t->artist, elem);
    }

    if (NULL != t->mb_track_id) {
        strncpy(t->mb_track_id, s->mb_track_id, MAX_PROPERTY_LENGTH);
    }
    if (NULL != t->mb_album_id) {
        strncpy(t->mb_album_id, s->mb_album_id, MAX_PROPERTY_LENGTH);
    }
    if (NULL != t->mb_artist_id) {
        strncpy(t->mb_artist_id, s->mb_artist_id, MAX_PROPERTY_LENGTH);
    }
    if (NULL != t->mb_album_artist_id) {
        strncpy(t->mb_album_artist_id, s->mb_album_artist_id, MAX_PROPERTY_LENGTH);
    }
    if (NULL != t->mb_spotify_id) {
        strncpy(t->mb_spotify_id, s->mb_spotify_id, MAX_PROPERTY_LENGTH);
    }

    t->length = s->length;
    t->position = s->position;
    t->start_time = s->start_time;
    t->play_time = s->play_time;
    t->track_number = s->track_number;

    _trace("scrobbler::copied_scrobble:%p->%p", s, t);
}

static bool scrobbles_equal(const struct scrobble *s, const struct scrobble *p)
{
    if ((NULL == s) && (NULL == p)) { return true; }

    if (NULL == s) { return false; }
    if (NULL == p) { return false; }

    if (s == p) { return true; }

    assert(s->title);
    assert(p->title);
    assert(s->album);
    assert(p->album);
    assert(s->artist);
    assert(p->artist);
    bool result = (
        (strncmp(s->title, p->title, strlen(s->title)) == 0)
        && (strncmp(s->album, p->album, strlen(s->album)) == 0)
        && (strncmp(s->artist[0], p->artist[0], strlen(s->artist[0])) == 0)
#if 0
        && (s->length == p->length)
        && (s->track_number == p->track_number)
#endif
        );
    _trace("scrobbler::check_scrobbles(%p:%p) %s", s, p, result ? "same" : "different");

    return result;
}

bool load_scrobble(struct scrobble *d, const struct mpris_properties *p)
{
    if (NULL == d) { return false; }
    if (NULL == p) { return false ; }
    if (NULL == p->metadata) { return false; }

    if (NULL == p->metadata->title || strlen(p->metadata->title) == 0) { return false; }
    if (NULL == p->metadata->album || strlen(p->metadata->album) == 0) { return false; }
    if (sb_count(p->metadata->artist) == 0 || NULL == p->metadata->artist[0] || strlen(p->metadata->artist[0]) == 0) { return false; }

    strncpy(d->title, p->metadata->title, MAX_PROPERTY_LENGTH);
    strncpy(d->album, p->metadata->album, MAX_PROPERTY_LENGTH);
    int artist_count = sb_count(p->metadata->artist);

    if (NULL != d->artist) {
        for (int i = 0; i < sb_count(d->artist); i++) {
            if (NULL != d->artist[i]) { free(d->artist[i]); }
        }
        sb_free(d->artist);
    }
    for (int i = 0; i < artist_count; i++) {
        char *elem = get_zero_string(MAX_PROPERTY_LENGTH);
        strncpy(elem, p->metadata->artist[i], strlen(p->metadata->artist[i]));

        sb_push(d->artist, elem);
    }

    if (p->metadata->length > 0) {
        d->length = p->metadata->length / 1000000.0f;
    } else {
        d->length = 59;
    }
    if (p->position > 0) {
        d->position = p->position / 1000000.0f;
    }
    d->scrobbled = false;
    d->track_number = p->metadata->track_number;
    d->start_time = p->metadata->timestamp;
    if (d->position == 0) {
        // TODO(marius): This is an awful way of doing this, but it's needed
        //               because Spotify doesn't populate the position correctly.
        //               These types of player based logic should be moved to their own functions. See #25
        time_t now = time(0);
        d->play_time = now - p->metadata->timestamp;
    } else {
        d->play_time = d->position;
    }

    // musicbrainz data
    if (NULL != p->metadata->mb_track_id && strlen(p->metadata->mb_track_id) > 0) {
        strncpy(d->mb_track_id, p->metadata->mb_track_id, strlen(p->metadata->mb_track_id));
    }

    if (NULL != p->metadata->mb_album_id && strlen(p->metadata->mb_album_id) > 0) {
        strncpy(d->mb_album_id, p->metadata->mb_album_id, MAX_PROPERTY_LENGTH);
    }

    if (NULL != p->metadata->mb_artist_id && strlen(p->metadata->mb_artist_id) > 0) {
        strncpy(d->mb_artist_id, p->metadata->mb_artist_id, MAX_PROPERTY_LENGTH);
    }

    if (NULL != p->metadata->mb_album_artist_id && strlen(p->metadata->mb_album_artist_id) > 0) {
        strncpy(d->mb_album_artist_id, p->metadata->mb_album_artist_id, MAX_PROPERTY_LENGTH);
    }

    // if this is spotify we add the track_id as the spotify_id
    if (NULL != p->metadata->track_id && strncmp(p->metadata->track_id, MPRIS_SPOTIFY_TRACK_ID_PREFIX, strlen(MPRIS_SPOTIFY_TRACK_ID_PREFIX)) == 0) {
        strncpy(d->mb_spotify_id, p->metadata->track_id + strlen(MPRIS_SPOTIFY_TRACK_ID_PREFIX), MAX_PROPERTY_LENGTH);
    }
    if (now_playing_is_valid(d)) {
        _trace("scrobbler::loaded_scrobble(%p)", d);
        _trace("  scrobble::title: %s", d->title);
        _trace("  scrobble::artist[%zu]: %s...", sb_count(d->artist), d->artist[0]);
        _trace("  scrobble::album: %s", d->album);
        _trace("  scrobble::length: %lu", d->length);
        _trace("  scrobble::position: %.2f", d->position);
        _trace("  scrobble::scrobbled: %s", d->scrobbled ? "yes" : "no");
        _trace("  scrobble::track_number: %u", d->track_number);
        _trace("  scrobble::start_time: %lu", d->start_time);
        _trace("  scrobble::play_time: %.2f", d->play_time);

        if (strlen(d->mb_track_id) > 0) {
            _trace("  scrobble::musicbrainz::track_id: %s", d->mb_track_id);
        }
        if (strlen(d->mb_artist_id) > 0) {
            _trace("  scrobble::musicbrainz::artist_id: %s", d->mb_artist_id);
        }
        if (strlen(d->mb_album_id) > 0) {
            _trace("  scrobble::musicbrainz::album_id: %s", d->mb_album_id);
        }
        if (strlen(d->mb_album_artist_id) > 0) {
            _trace("  scrobble::musicbrainz::album_artist_id: %s", d->mb_album_artist_id);
        }
        if (strlen(d->mb_spotify_id) > 0) {
            _trace("  scrobble::musicbrainz::spotify_id: %s", d->mb_spotify_id);
        }
    }
    return true;
}

bool scrobbles_append(struct mpris_player *player, const struct scrobble *track)
{

    if (NULL == player) { return false; }
    if (NULL == track) { return false; }

    struct scrobble *n = scrobble_new();
    scrobble_copy(n, track);

    // TODO(marius) this looks very fishy, usually current and properties are equal
    int queue_count = sb_count(player->queue);
    if (queue_count > 0) {
        struct scrobble *current = player->queue[queue_count - 1];
        _debug("scrobbler::queue_top[%u]: %p", queue_count, current);
        if (scrobbles_equal(current, n)) {
            _debug("scrobbler::queue: skipping existing scrobble(%p): %s//%s//%s", n, n->title, n->artist[0], n->album);
            scrobble_free(n);
            return false;
        }
    }

    sb_push(player->queue, n);
    _trace("scrobbler::queue_push_scrobble(%p//%-4u) %s//%s//%s", n, queue_count, n->title, n->artist[0], n->album);
    _debug("scrobbler::new_queue_length: %zu", sb_count(player->queue));
    _trace("scrobbler::copied_current:(%p::%p)", player->properties, player->current);

    mpris_properties_zero(player->properties, true);

    return true;
}

void debug_event(const struct mpris_event *e)
{
    _debug("scrobbler::checking_volume_changed:\t\t%3s", e->volume_changed ? "yes" : "no");
    _debug("scrobbler::checking_position_changed:\t\t%3s", e->position_changed ? "yes" : "no");
    _debug("scrobbler::checking_playback_status_changed:\t%3s", e->playback_status_changed ? "yes" : "no");
    _debug("scrobbler::checking_track_changed:\t\t%3s", e->track_changed ? "yes" : "no");
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
            return;
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

size_t scrobbles_consume_queue(struct scrobbler *scrobbler, struct scrobble **inc_tracks)
{
    if (NULL == scrobbler) { return 0; }
    if (NULL == inc_tracks) { return 0; }

    size_t consumed = 0;
    int queue_length = sb_count(inc_tracks);
    _trace("scrobbler::queue_length: %u", queue_length);

    struct scrobble **tracks = NULL;

    if (queue_length == 0) { return consumed; }

    int queue_count = sb_count(inc_tracks);
    for (int pos = 0; pos < queue_count; pos++) {
        struct scrobble *current = inc_tracks[pos];
        if (scrobble_is_valid(current)) {
            _info("scrobbler::scrobble::valid:(%p//%zu) %s//%s//%s", current, pos, current->title, current->artist[0], current->album);
            sb_push(tracks, current);
        } else {
            _warn("scrobbler::scrobble::invalid:(%p//%zu) %s//%s//%s", current, pos, current->title, current->artist[0], current->album);
        }
    }
    api_request_do(scrobbler, (const struct scrobble**)tracks, api_build_request_scrobble);

    scrobbles_free(inc_tracks);
    sb_free(tracks);
    return consumed;
}

#endif // MPRIS_SCROBBLER_SCROBBLE_H
