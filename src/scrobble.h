/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SCROBBLE_H
#define MPRIS_SCROBBLER_SCROBBLE_H

#include <time.h>

#define NOW_PLAYING_DELAY 65 //seconds
#define MIN_TRACK_LENGTH  30 // seconds

#define LASTFM_API_KEY "990909c4e451d6c1ee3df4f5ee87e6f4"
#define LASTFM_API_SECRET "8bde8774564ef206edd9ef9722742a72"

#define LIBREFM_API_KEY "299dead99beef992"
#define LIBREFM_API_SECRET "c0ffee1511fe"

char *get_player_namespace(DBusConnection *);
void get_mpris_properties(DBusConnection*, const char*, mpris_properties*, struct mpris_event*);
struct events* events_new(void);
dbus *dbus_connection_init(struct state*);
void state_loaded_properties(struct state* , mpris_properties*, const struct mpris_event*);

struct scrobbler {
    struct api_credentials **credentials;
    size_t credentials_length;
};

#if 0
const char* get_api_status_label (api_return_codes code)
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
    //if (NULL != s->title) { free(s->title); }
    s->title = NULL;
    //if (NULL != s->album) { free(s->album); }
    s->album = NULL;
    //if (NULL != s->artist) { free(s->artist); }
    s->artist = NULL;
    s->scrobbled = false;
    s->length = 0;
    s->position = 0;
    s->start_time = 0;
    s->track_number = 0;

    _trace("mem::inited_scrobble(%p)", s);
}

struct scrobble* scrobble_new(void)
{
    struct scrobble *result = malloc(sizeof(struct scrobble));
    scrobble_init(result);

    return result;
}

void scrobble_free(struct scrobble *s)
{
    if (NULL == s) return;

    if (NULL != s->title) {
        _trace("mem::scrobble::freeing_title(%p): %s", s->title, s->title);
        free(s->title);
        s->title = NULL;
    }
    if (NULL != s->album) {
        _trace("mem::scrobble::freeing_album(%p): %s", s->album, s->album);
        free(s->album);
        s->album = NULL;
    }
    if (NULL != s->artist) {
        _trace("mem::scrobble:freeing_artist(%p): %s", s->artist, s->artist);
        free(s->artist);
        s->artist = NULL;
    }

    _trace("mem::freeing_scrobble(%p)", s);
    free(s);
}

static void scrobbler_free(struct scrobbler *s)
{
    if (NULL == s) { return; }
    free(s);
}

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

static void mpris_player_free(struct mpris_player *player)
{
    _trace("mem::freeing_player(%p)::queue_length:%u", player, player->queue_length);
    for (size_t i = 0; i < player->queue_length; i++) {
        scrobbles_remove(player->queue, player->queue_length, i);
    }
    if (NULL != player->mpris_name) { free(player->mpris_name); }
    if (NULL != player->properties) { mpris_properties_free(player->properties); }
    if (NULL != player->changed) { free(player->changed); }

    free (player);
}

void events_free(struct events *);
void dbus_close(struct state *);
void state_free(struct state *s)
{
    if (NULL != s->player) { mpris_player_free(s->player); }
    if (NULL != s->dbus) { dbus_close(s); }
    if (NULL != s->events) { events_free(s->events); }
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
    struct mpris_player *result = malloc(sizeof(struct mpris_player));

    return (result);
}

static void mpris_player_init(struct mpris_player *player, DBusConnection *conn)
{
    _trace("mem::initing_player(%p)", player);
    player->queue_length = 0;
    player->mpris_name = NULL;
    player->changed = calloc(1, sizeof(struct mpris_event));

    if (NULL != conn) {
        player->properties = mpris_properties_new();

        player->mpris_name = get_player_namespace(conn);
        if (NULL != player->mpris_name ) {
            get_mpris_properties(conn, player->mpris_name, player->properties, player->changed);
        }
    }
    _trace("mem::inited_player(%p)", player);
}

static void state_init(struct state *s)
{
    extern struct configuration global_config;

    _trace("mem::initing_state(%p)", s);
    s->scrobbler = scrobbler_new();
    s->player = mpris_player_new();

    scrobbler_init(s->scrobbler, &global_config);

    s->events = events_new();
    s->dbus = dbus_connection_init(s);

    mpris_player_init(s->player, s->dbus->conn);
    state_loaded_properties(s, s->player->properties, s->player->changed);

    _trace("mem::inited_state(%p)", s);
}

struct state* state_new(void)
{
    struct state *result = malloc(sizeof(struct state));
    if (NULL == result) { return NULL; }

    state_init(result);

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

    cpstr(&t->title, s->title, MAX_PROPERTY_LENGTH);
    cpstr(&t->album, s->album, MAX_PROPERTY_LENGTH);
    cpstr(&t->artist, s->artist, MAX_PROPERTY_LENGTH);

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

void scrobbles_append(struct mpris_player *player, const struct mpris_properties *m)
{
    if (player->queue_length > 0 && mpris_properties_equals(player->current, m)) {
        _debug("scrobbler::queue: skipping existing scrobble(%p)", m);
        return;
    }
    struct mpris_properties *n = mpris_properties_new();
    mpris_properties_copy(n, m);

    if (player->queue_length > QUEUE_MAX_LENGTH) {
        _error("scrobbler::queue_length_exceeded: please check connection");
        mpris_properties_free(n);
        return;
    }
    for (size_t i = player->queue_length; i > 0; i--) {
        size_t idx = i - 1;
        struct mpris_properties *to_move = player->queue[idx];
        if(to_move == m) { continue; }
        player->queue[i] = to_move;
        if (NULL == to_move) { continue; }
        mpris_metadata *d = to_move->metadata;
        _trace("scrobbler::queue_move_scrobble(%p//%u<-%u) %s//%s//%s", to_move, i, i-1, d->title, d->artist, d->album);
    }
    player->queue_length++;
    mpris_metadata *p = n->metadata;
    _trace("scrobbler::queue_push_scrobble(%p//%4u) %s//%s//%s", p, 0, p->title, p->artist, p->album);
    _debug("scrobbler::new_queue_length:%u", player->queue_length);

    player->queue[0] = n;
    mpris_properties_zero(player->properties, true);
}

#if 0
static struct mpris_properties* scrobbles_pop(struct mpris_player *player)
{
    struct mpris_properties *last = mpris_properties_new();

    size_t cnt = player->queue_length - 1;
    if (NULL == player->queue[cnt]) { return NULL; }
    mpris_properties_copy(last, player->queue[cnt]);
    scrobbles_remove(player->queue, player->queue_length, cnt);

    return last;
}

static struct mpris_properties* scrobbles_peek_queue(struct mpris_player *player, size_t i)
{
    _trace("scrobbler::peeking_at:%d: (%p)", i, player->queue[i]);
    if (i <= player->queue_length && NULL != player->queue[i]) {
        return player->queue[i];
    }
    return NULL;
}
#endif

void debug_event(struct mpris_event* e)
{
    _debug("scrobbler::checking_volume_changed:\t\t%3s", e->volume_changed ? "yes" : "no");
    _debug("scrobbler::checking_position_changed:\t\t%3s", e->volume_changed ? "yes" : "no");
    _debug("scrobbler::checking_playback_status_changed:\t%3s", e->playback_status_changed ? "yes" : "no");
    _debug("scrobbler::checking_track_changed:\t\t%3s", e->track_changed ? "yes" : "no");
}

void load_scrobble(struct scrobble* d, const mpris_properties *p)
{
    if (NULL == d) { return; }
    if (NULL == p) { return; }
    if (NULL == p->metadata) { return; }

    time_t tstamp = time(0);
    //if (NULL != d->title) { free(d->title); }
    cpstr(&d->title, p->metadata->title, MAX_PROPERTY_LENGTH);
    //if (NULL != d->album) { free(d->album); }
    cpstr(&d->album, p->metadata->album, MAX_PROPERTY_LENGTH);
    //if (NULL != d->artist) { free(d->artist); }
    cpstr(&d->artist, p->metadata->artist, MAX_PROPERTY_LENGTH);

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
    d->start_time = tstamp;
    d->play_time = d->position;
    if (now_playing_is_valid(d)) {
        _trace("scrobbler::loading_scrobble(%p)", d);
        _trace("  scrobble::title: %s", d->title);
        _trace("  scrobble::artist: %s", d->artist);
        _trace("  scrobble::album: %s", d->album);
        _trace("  scrobble::length: %lu", d->length);
        _trace("  scrobble::position: %.2f", d->position);
        _trace("  scrobble::scrobbled: %s", d->scrobbled ? "yes" : "no");
        _trace("  scrobble::track_number: %u", d->track_number);
        _trace("  scrobble::start_time: %lu", d->start_time);
        _trace("  scrobble::play_time: %.2f", d->play_time);
    }
}

static bool lastfm_scrobble(struct scrobbler *s, const struct scrobble *track)
{
    if (NULL == s) { return false; }
    if (NULL == track) { return false; }

    if (!scrobble_is_valid(track)) {
        _warn("scrobbler::invalid_scrobble(%p): %s//%s//%s",
            track,
            (NULL != track->title ? track->title : "(unknown)"),
            (NULL != track->artist ? track->artist : "(unknown)"),
            (NULL != track->album ? track->album : "(unknown)"));
        return false;
    }

    _info("scrobbler::scrobble: %s//%s//%s", track->title, track->artist, track->album);

    if (s->credentials == 0) { return false; }
    const struct scrobble *tracks[1];
    tracks[0] = track;

    for (size_t i = 0; i < s->credentials_length; i++) {
        struct api_credentials *cur = s->credentials[i];
        if (NULL == cur) { continue; }
        if (s->credentials[i]->enabled) {
            CURL *curl = curl_easy_init();
            struct http_request *req = api_build_request_scrobble(tracks, 1, curl, cur->end_point);
            struct http_response *res = http_response_new();

            // TODO: do something with the response to see if the api call was successful
            bool ok = api_post_request(curl, req, res);

            http_request_free(req);
            http_response_free(res);
            if (ok) {
                _info("api::submitted_to[%s] %s", get_api_type_label(cur->end_point), "ok");
            } else {
                _error("api::submitted_to[%s] %s", get_api_type_label(cur->end_point), "nok");
            }
            curl_easy_cleanup(curl);
        }
    }
    return true;
}

static bool lastfm_now_playing(struct scrobbler *s, const struct scrobble *track)
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
        if (s->credentials[i]->enabled) {
            CURL *curl = curl_easy_init();
            struct http_request *req = api_build_request_now_playing(track, curl, cur->end_point);
            struct http_response *res = http_response_new();

            // TODO: do something with the response to see if the api call was successful
            enum api_return_statuses status = api_post_request(curl, req, res);

            http_request_free(req);
            http_response_free(res);
            curl_easy_cleanup(curl);
            if (status == status_ok) {
                _info("api::submitted_to[%s] %s", get_api_type_label(cur->end_point), "ok");
            } else {
                _error("api::submitted_to[%s] %s", get_api_type_label(cur->end_point), "nok");
            }
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

    for (size_t pos = 0; pos < queue_length; pos++) {
        struct mpris_properties *properties = player->queue[pos];

        struct scrobble* current = scrobble_new();
        load_scrobble(current, properties);
        if (scrobble_is_valid(current)) {
            _trace("scrobbler::scrobble_pos(%p//%i): valid", current, pos);
            if (!current->scrobbled) {
                current->scrobbled = lastfm_scrobble(scrobbler, current);

                if (current->scrobbled) { consumed++; }
            }
        } else {
            _warn("scrobbler::invalid_scrobble:pos(%p//%i) %s//%s//%s", current, pos, current->title, current->artist, current->album);
        }
        scrobbles_remove(player->queue, player->queue_length, pos);
        player->queue_length--;

        scrobble_free(current);
    }
    if (consumed > 0) {
        _trace("scrobbler::queue_consumed: %i, new_queue_length: %i", consumed, player->queue_length);
    }
    return consumed;
}

#if 0
static bool scrobbles_has_previous(const struct mpris_player *player)
{
    if(NULL != player) { return false; }
    return (NULL != player->previous);
}
#endif

#endif // MPRIS_SCROBBLER_SCROBBLE_H
