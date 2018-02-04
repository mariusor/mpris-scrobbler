/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SCROBBLE_H
#define MPRIS_SCROBBLER_SCROBBLE_H

#include <time.h>

#define NOW_PLAYING_DELAY 65 //seconds
#define MIN_TRACK_LENGTH  30 // seconds
#define MPRIS_SPOTIFY_TRACK_ID_PREFIX                          "spotify:track:"

char *get_player_namespace(DBusConnection *);
void get_mpris_properties(DBusConnection*, const char*, struct mpris_properties*, struct mpris_event*);

struct dbus *dbus_connection_init(struct state*);
void state_loaded_properties(struct state*, struct mpris_properties*, const struct mpris_event*);

struct scrobbler {
    struct api_credentials **credentials;
    size_t credentials_length;
};

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
    s->artist = get_zero_string(MAX_PROPERTY_LENGTH);

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
        if (strlen(s->artist) > 0) { _trace2("mem::scrobble::free::artist(%p): %s", s->artist, s->artist); }
        free(s->artist);
        s->artist = NULL;
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

static void scrobbler_free(struct scrobbler *s)
{
    if (NULL == s) { return; }
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

static void mpris_player_free(struct mpris_player *player)
{
    if (NULL == player) { return; }

    _trace2("mem::free::player(%p)::queue_length:%u", player, player->queue_length);
    for (size_t i = 0; i < player->queue_length; i++) {
        //scrobbles_remove(player->queue, player->queue_length, i);
        mpris_properties_free(player->queue[i]);
        player->queue[i] = NULL;
    }
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

struct scrobbler *scrobbler_new(void)
{
    struct scrobbler *result = malloc(sizeof(struct scrobbler));

    return (result);
}

static void scrobbler_init(struct scrobbler *s, struct configuration *config)
{
    s->credentials = config->credentials;
    s->credentials_length = config->credentials_length;
}

static struct mpris_player *mpris_player_new(void)
{
    struct mpris_player *result = calloc(1, sizeof(struct mpris_player));

    return (result);
}

static void mpris_player_init(struct mpris_player *player, DBusConnection *conn)
{
    _trace2("mem::initing_player(%p)", player);
    player->queue_length = 0;
    player->mpris_name = NULL;
    player->changed = calloc(1, sizeof(struct mpris_event));

    if (NULL != conn) {
        player->properties = mpris_properties_new();
        player->current = mpris_properties_new();

        player->mpris_name = get_player_namespace(conn);
        if (NULL != player->mpris_name) {
            get_mpris_properties(conn, player->mpris_name, player->properties, player->changed);
        }
    }
    _trace2("mem::inited_player(%p)", player);
}

struct events *events_new(void);
void events_init(struct events*, struct sighandler_payload*);
bool state_init(struct state *s, struct sighandler_payload *sig_data)
{
    _trace2("mem::initing_state(%p)", s);
    s->scrobbler = scrobbler_new();
    s->player = mpris_player_new();

    scrobbler_init(s->scrobbler, sig_data->config);

    s->events = events_new();
    events_init(s->events, sig_data);
    s->dbus = dbus_connection_init(s);

    if (NULL == s->dbus) { return false; }
    mpris_player_init(s->player, s->dbus->conn);
    state_loaded_properties(s, s->player->properties, s->player->changed);

    _trace2("mem::inited_state(%p)", s);

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
    if (NULL == s->artist) { return false; }
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
//        s->length >= LASTFM_MIN_TRACK_LENGTH &&
//        s->play_time >= (s->length / 2) &&
        s->scrobbled == false &&
        strlen(s->title) > 0 &&
        strlen(s->artist) > 0 &&
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
            (NULL != m->artist && strlen(m->artist) > 0) ||
            (NULL != m->album && strlen(m->album) > 0)
        ) &&
//        last_playing_time > 0 &&
//        difftime(current_time, last_playing_time) >= LASTFM_NOW_PLAYING_DELAY &&
        m->length > 0
    );
}

#if 0
static void scrobble_copy (struct scrobble *t, const struct scrobble *s)
{
    if (NULL == t) { return; }
    if (NULL == s) { return; }
//    if (NULL == s->title) { return; }
//    if (NULL == s->artist) { return; }
//    if (NULL == s->album) { return; }

    strncpy(t->title, s->title, MAX_PROPERTY_LENGTH);
    strncpy(t->album, s->album, MAX_PROPERTY_LENGTH);
    strncpy(t->artist, s->artist, MAX_PROPERTY_LENGTH);

    t->length = s->length;
    t->position = s->position;
    t->start_time = s->start_time;
    t->play_time = s->play_time;
    t->track_number = s->track_number;
}

static bool scrobbles_equals(const struct scrobble *s, const struct scrobble *p)
{
    if (NULL == s) { return false; }
    if (NULL == p) { return false; }
    if (s == p) { return true; }

    bool result = (
        (strncmp(s->title, p->title, strlen(s->title)) == 0) &&
        (strncmp(s->album, p->album, strlen(s->album)) == 0) &&
        (strncmp(s->artist, p->artist, strlen(s->artist)) == 0) &&
        (s->length == p->length) &&
        (s->track_number == p->track_number) /*&&
        (s->start_time == p->start_time)*/
    );
    _trace("scrobbler::check_scrobbles(%p:%p) %s", s, p, result ? "same" : "different");

    return result;
}
#endif

bool scrobbles_append(struct mpris_player *player, const struct mpris_properties *m)
{
    // todo: [om] this looks very fishy, usually current and properties are equal
    if (player->queue_length > 0) {
        struct mpris_properties *current = player->queue[player->queue_length - 1];
        _debug("scrobbler::top_queue[%u]: %p", player->queue_length, current);
        if (mpris_properties_equals(current, m)) {
            _debug("scrobbler::queue: skipping existing scrobble(%p)", m);
            return false;
        }
    }

    struct mpris_properties *n = mpris_properties_new();
    mpris_properties_copy(n, m);

    if (player->queue_length >= QUEUE_MAX_LENGTH) {
        _error("scrobbler::queue_length_exceeded: please check connection");
        mpris_properties_free(n);
        return false;
    }
    for (size_t i = player->queue_length; i > 0; i--) {
        size_t idx = i - 1;
        struct mpris_properties *to_move = player->queue[idx];
        if(to_move == m) { continue; }
        player->queue[i] = to_move;
        if (NULL == to_move) { continue; }
        struct mpris_metadata *d = to_move->metadata;
        _trace("scrobbler::queue_move_scrobble(%p//%u<-%u) %s//%s//%s", to_move, i, i-1, d->title, d->artist, d->album);
    }
    player->queue_length++;
    struct mpris_metadata *p = n->metadata;
    _trace("scrobbler::queue_push_scrobble(%p//%4u) %s//%s//%s", p, 0, p->title, p->artist, p->album);
    _debug("scrobbler::new_queue_length:%u", player->queue_length);

    player->queue[0] = n;

#if 0
    mpris_properties_zero(player->current, true);
    mpris_properties_copy(player->current, player->properties);
#endif

    _trace("scrobbler::copied_current:(%p::%p)", player->properties, player->current);

    mpris_properties_zero(player->properties, true);

    return true;
}

#if 0
static struct mpris_properties *scrobbles_pop(struct mpris_player *player)
{
    struct mpris_properties *last = mpris_properties_new();

    size_t cnt = player->queue_length - 1;
    if (NULL == player->queue[cnt]) { return NULL; }
    mpris_properties_copy(last, player->queue[cnt]);
    scrobbles_remove(player->queue, player->queue_length, cnt);

    return last;
}

static struct mpris_properties *scrobbles_peek_queue(struct mpris_player *player, size_t i)
{
    _trace("scrobbler::peeking_at:%d: (%p)", i, player->queue[i]);
    if (i <= player->queue_length && NULL != player->queue[i]) {
        return player->queue[i];
    }
    return NULL;
}
#endif

void debug_event(struct mpris_event *e)
{
    _debug("scrobbler::checking_volume_changed:\t\t%3s", e->volume_changed ? "yes" : "no");
    _debug("scrobbler::checking_position_changed:\t\t%3s", e->position_changed ? "yes" : "no");
    _debug("scrobbler::checking_playback_status_changed:\t%3s", e->playback_status_changed ? "yes" : "no");
    _debug("scrobbler::checking_track_changed:\t\t%3s", e->track_changed ? "yes" : "no");
}

void load_scrobble(struct scrobble *d, const struct mpris_properties *p)
{
    if (NULL == d) { return; }
    if (NULL == p) { return; }
    if (NULL == p->metadata) { return; }

    strncpy(d->title, p->metadata->title, MAX_PROPERTY_LENGTH);
    strncpy(d->album, p->metadata->album, MAX_PROPERTY_LENGTH);
    strncpy(d->artist, p->metadata->artist, MAX_PROPERTY_LENGTH);

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
    d->play_time = d->position;

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
        _trace("  scrobble::artist: %s", d->artist);
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
}

static bool scrobbler_scrobble(struct scrobbler *s, const struct scrobble *tracks[], const size_t track_count)
{
    if (NULL == s) { return false; }

    if (s->credentials == 0) { return false; }

    for (size_t i = 0; i < s->credentials_length; i++) {
        struct api_credentials *cur = s->credentials[i];
        if (NULL == cur) { continue; }
        if (s->credentials[i]->enabled) {
            if (NULL == cur->session_key) {
                _warn("scrobbler::invalid_service[%s]: missing session key", get_api_type_label(cur->end_point));
                return false;
            }
            CURL *curl = curl_easy_init();
            struct http_request *req = api_build_request_scrobble(tracks, track_count, curl, cur);
            struct http_response *res = http_response_new();

            // TODO: do something with the response to see if the api call was successful
            enum api_return_status ok = api_post_request(curl, req, res);

            if (ok == status_ok) {
                _info("api::submitted_to[%s] %s", get_api_type_label(cur->end_point), "ok");
            } else {
                _error("api::submitted_to[%s] %s", get_api_type_label(cur->end_point), "nok");
            }
            curl_easy_cleanup(curl);
            http_request_free(req);
            http_response_free(res);
        }
    }
    return true;
}

static bool scrobbler_now_playing(struct scrobbler *s, const struct scrobble *track)
{
    if (NULL == s) { return false; }
    if (NULL == track) { return false; }

    if (!now_playing_is_valid(track)) {
        _warn("scrobbler::invalid_now_playing(%p): %s//%s//%s",
            track,
            (NULL != track->title ? track->title : "(unknown)"),
            (NULL != track->artist ? track->artist : "(unknown)"),
            (NULL != track->album ? track->album : "(unknown)"));
        return false;
    }

    _info("scrobbler::now_playing: %s//%s//%s", track->title, track->artist, track->album);

    if (s->credentials == 0) { return false; }
    for (size_t i = 0; i < s->credentials_length; i++) {
        struct api_credentials *cur = s->credentials[i];
        if (NULL == cur) { continue; }
        if (cur->enabled) {
            if (NULL == cur->session_key) {
                _warn("scrobbler::invalid_service[%s]: missing session key", get_api_type_label(cur->end_point));
                return false;
            }
            CURL *curl = curl_easy_init();
            struct http_request *req = api_build_request_now_playing(track, curl, cur);
            struct http_response *res = http_response_new();

            enum api_return_status status = api_post_request(curl, req, res);

            if (status == status_ok) {
                _info("api::submitted_to[%s] %s", get_api_type_label(cur->end_point), "ok");
            } else {
                _error("api::submitted_to[%s] %s", get_api_type_label(cur->end_point), "nok");
            }
            http_request_free(req);
            http_response_free(res);
            curl_easy_cleanup(curl);
        }
    }
    return true;
}

size_t scrobbles_consume_queue(struct scrobbler *scrobbler, struct mpris_player *player)
{
    if (NULL == scrobbler) { return 0; }
    if (NULL == player) { return 0; }

    size_t consumed = 0;
    size_t queue_length = player->queue_length;
    _trace("scrobbler::queue_length: %i", queue_length);

    if (queue_length == 0) { return consumed; }
    const struct scrobble *tracks[QUEUE_MAX_LENGTH]; // = calloc(player->queue_length, sizeof(struct scrobble));

    size_t track_count = 0;
    for (size_t pos = 0; pos < queue_length; pos++) {
        struct mpris_properties *properties = player->queue[pos];
        tracks[track_count] = scrobble_new();

        struct scrobble *current = (struct scrobble*)tracks[track_count];
        load_scrobble(current, properties);
        if (scrobble_is_valid(current)) {
            _info("scrobbler::scrobble::valid:(%p//%zu) %s//%s//%s", current, pos, current->title, current->artist, current->album);
            track_count++;
        } else {
            _warn("scrobbler::scrobble::invalid:(%p//%zu) %s//%s//%s", current, pos, current->title, current->artist, current->album);
        }
        //scrobbles_remove(player->queue, player->queue_length, pos);
        mpris_properties_free(properties);
        player->queue[pos] = NULL;
        player->queue_length--;
    }
    consumed = scrobbler_scrobble(scrobbler, tracks, track_count);
    for (size_t i = 0; i < track_count; i++) {
        struct scrobble *current = (struct scrobble *)tracks[i];
        scrobble_free(current);
    }
    if (consumed > 0) {
        _debug("scrobbler::queue_consumed: %lu, new_queue_length: %lu", consumed, player->queue_length);
    }
    return consumed;
}

#endif // MPRIS_SCROBBLER_SCROBBLE_H
