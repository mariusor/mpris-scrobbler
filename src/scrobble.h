/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef SCROBBLE_H
#define SCROBBLE_H

#include <time.h>

#define NOW_PLAYING_DELAY 65 //seconds
#define MIN_TRACK_LENGTH  30 // seconds

#define LASTFM_API_KEY "990909c4e451d6c1ee3df4f5ee87e6f4"
#define LASTFM_API_SECRET "8bde8774564ef206edd9ef9722742a72"

#define LIBREFM_API_KEY "299dead99beef992"
#define LIBREFM_API_SECRET "c0ffee1511fe"

char *get_player_namespace(DBusConnection *);
void get_mpris_properties(DBusConnection*, const char*, mpris_properties*);
void add_event_ping(struct state *);
struct events* events_new();
dbus *dbus_connection_init(struct state*);
void state_loaded_properties(struct state* , mpris_properties*);

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

void scrobble_init(scrobble *s)
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

scrobble* scrobble_new()
{
    scrobble *result = malloc(sizeof(scrobble));
    scrobble_init(result);

    return result;
}

void scrobble_free(scrobble *s)
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

void scrobbler_free(struct scrobbler *s)
{
    if (NULL == s) { return; }
    curl_easy_cleanup(s->curl);
    free(s);
}

void mpris_player_free(struct mpris_player *player)
{
    _trace("mem::freeing_player(%p)::queue_length:%u", player, player->queue_length);
    for (unsigned i = 0; i < player->queue_length; i++) {
        scrobble_free(player->queue[i]);
    }
    if (NULL != player->mpris_name) { free(player->mpris_name); }
    if (NULL != player->properties) { mpris_properties_unref(player->properties); }

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

struct scrobbler *scrobbler_new()
{
    struct scrobbler *result = malloc(sizeof(struct scrobbler));

    return (result);
}

void scrobbler_init(struct scrobbler *s, struct configuration *config)
{
    s->credentials = config->credentials;
    s->credentials_length = config->credentials_length;
    s->curl = curl_easy_init();
}

struct mpris_player *mpris_player_new()
{
    struct mpris_player *result = malloc(sizeof(struct mpris_player));

    return (result);
}

void mpris_player_init(struct mpris_player *player, DBusConnection *conn)
{
    _trace("mem::initing_player(%p)", player);
    player->queue_length = 0;
    player->player_state = stopped;
    player->mpris_name = NULL;

    if (NULL != conn) {
        player->properties = mpris_properties_new();

        player->mpris_name = get_player_namespace(conn);
        if (NULL != player->mpris_name ) {
            get_mpris_properties(conn, player->mpris_name, player->properties);
        }
    }
    _trace("mem::inited_player(%p)", player);
}

void state_init(struct state *s)
{
    extern struct configuration global_config;

    _trace("mem::initing_state(%p)", s);
    s->scrobbler = scrobbler_new();
    s->player = mpris_player_new();

    scrobbler_init(s->scrobbler, &global_config);

    s->events = events_new();
    s->dbus = dbus_connection_init(s);
    mpris_player_init(s->player, s->dbus->conn);

#if 0
    add_event_ping(s);
#endif

    _trace("mem::inited_state(%p)", s);
}

struct state* state_new()
{
    struct state *result = malloc(sizeof(struct state));
    if (NULL == result) { return NULL; }

    state_init(result);

    return result;
}

bool scrobble_is_valid(const scrobble *s)
{
    if (NULL == s) { return false; }
    if (NULL == s->title) { return false; }
    if (NULL == s->album) { return false; }
    if (NULL == s->artist) { return false; }
    if (s->scrobbled) { return false; }
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

bool now_playing_is_valid(const scrobble *m/*, const time_t current_time, const time_t last_playing_time*/) {
    return (
        NULL != m &&
        NULL != m->title && strlen(m->title) > 0 &&
        NULL != m->artist && strlen(m->artist) > 0 &&
        NULL != m->album && strlen(m->album) > 0 &&
//        last_playing_time > 0 &&
//        difftime(current_time, last_playing_time) >= LASTFM_NOW_PLAYING_DELAY &&
        m->length > 0
    );
}

void scrobble_copy (scrobble *t, const scrobble *s)
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

static bool scrobbles_equals(const scrobble *s, const scrobble *p)
{
    if (NULL == s) { return false; }
    if (NULL == p) { return false; }

    bool result = (
        (strncmp(s->title, p->title, strlen(s->title)) == 0) &&
        (strncmp(s->album, p->album, strlen(s->album)) == 0) &&
        (strncmp(s->artist, p->artist, strlen(s->artist)) == 0) &&
        (s->length == p->length) &&
        (s->track_number == p->track_number) /*&&
        (s->start_time == p->start_time)*/
    );
    _trace("last.fm::check_scrobbles(%p:%p) %s", s, p, result ? "same" : "different");

    return result;
}

void scrobbles_append(struct mpris_player *player, const scrobble *m)
{
#if 0
    if (m->play_time <= (m->length / 2.0f)) {
        _trace("last.fm::not_enough_play_time: %.2f < %.2f", m->play_time, m->length/2.0f);
        return;
    }
#endif
    if (player->queue_length > 0 /*&& scrobbles_equals(s->current, m)*/) {
        _debug("last.fm::queue: skipping existing scrobble(%p)",m);
        return;
    }
    scrobble *n = scrobble_new();
    scrobble_copy(n, m);

    if (player->queue_length > QUEUE_MAX_LENGTH) {
        _error("last.fm::queue_length_exceeded: please check connection");
        scrobble_free(n);
        return;
    }
    for (size_t i = player->queue_length; i > 0; i--) {
        scrobble *to_move = player->queue[i-1];
        if(to_move == m) { continue; }
        player->queue[i] = to_move;
        _debug("last.fm::queue_move_scrobble(%p//%u<-%u) %s//%s//%s", to_move, i, i-1, to_move->title, to_move->artist, to_move->album);
    }
    player->queue_length++;
    _debug("last.fm::queue_push_scrobble(%p//%4u) %s//%s//%s", n, 0, n->title, n->artist, n->album);
    player->queue[0] = n;
}

void scrobbles_remove(struct mpris_player *player, size_t pos)
{
    scrobble *last = player->queue[pos];
    _debug("last.fm::popping_scrobble(%p//%u) %s//%s//%s", player->queue[pos], pos, last->title, last->artist, last->album);
    if (pos >= player->queue_length) { return; }

    scrobble_free(player->queue[pos]);

    player->queue_length--;
    player->previous = player->queue[pos-1];
}

scrobble* scrobbles_pop(struct mpris_player *player)
{
    scrobble *last = scrobble_new();

    size_t cnt = player->queue_length - 1;
    if (NULL == player->queue[cnt]) { return NULL; }
    scrobble_copy(last, player->queue[cnt]);
    scrobbles_remove(player, cnt);

    return last;
}

scrobble* scrobbles_peek_queue(struct mpris_player *player, size_t i)
{
    _trace("last.fm::peeking_at:%d: (%p)", i, player->queue[i]);
    if (i <= player->queue_length && NULL != player->queue[i]) {
        return player->queue[i];
    }
    return NULL;
}

void load_event(mpris_event* e, const mpris_properties *p, const struct state *state)
{
    if (NULL == e) { goto _return; }
    if (NULL == p) { goto _return; }
    if (NULL == p->metadata) { goto _return; }
    e->player_state = get_mpris_playback_status(p);
    e->track_changed = false;
    e->volume_changed = false;
    e->playback_status_changed = false;

    if (NULL == state) { goto _return; }

    const struct mpris_player *player = state->player;

    e->playback_status_changed = (e->player_state != player->player_state);

    mpris_properties *last = player->properties;
    if (NULL == last) { goto _return; }
    if (NULL == last->metadata) { goto _return; }

    if (last->volume != p->volume) {
        e->volume_changed = true;
    }
    if (
        (NULL == last->metadata->title) &&
        (NULL == last->metadata->album) &&
        (NULL == last->metadata->artist)
    ) {
        if (
            (NULL == p->metadata->title) &&
            (NULL == p->metadata->album) &&
            (NULL == p->metadata->artist)
        ) {
            e->track_changed = false;
        } else {
            e->track_changed = true;
        }
    }
    if (
        (NULL != p->metadata->title) &&
        (NULL != last->metadata->title)
    ) {
        e->track_changed = strncmp(p->metadata->title, last->metadata->title, strlen(p->metadata->title));
    }
    if (
        (NULL != p->metadata->artist) &&
        (NULL != last->metadata->artist)
    ) {
        e->track_changed = strncmp(p->metadata->artist, last->metadata->artist, strlen(p->metadata->artist));
    }
    if (
        (NULL != p->metadata->album) &&
        (NULL != last->metadata->album)
    ) {
        e->track_changed = strncmp(p->metadata->album, last->metadata->album, strlen(p->metadata->album));
    }
    if (last && p) {
        _debug("last.fm::checking_volume_changed:\t\t%3s %.2f -> %.2f",
             e->volume_changed ? "yes" : "no",
             last->volume, p->volume);
    } else {
        _info("last.fm::checking_volume_changed:\t\t%3s", e->volume_changed ? "yes" : "no");
    }
    if (last && p) {
        _debug("last.fm::checking_playback_status_changed:\t%3s %s -> %s",
             e->playback_status_changed ? "yes" : "no", last->playback_status, p->playback_status);
    } else {
        _info("last.fm::checking_playback_status_changed:\t%3s", e->playback_status_changed ? "yes" : "no");
    }
    if (last && p) {
        _debug("last.fm::checking_track_changed:\t\t%3s %s -> %s",
            e->track_changed ? "yes" : "no", last->metadata->title, p->metadata->title);
    } else {
        _info("last.fm::checking_track_changed:\t\t%3s", e->track_changed ? "yes" : "no");
    }
_return:
    return;
}

void load_scrobble(scrobble* d, const mpris_properties *p)
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
        _debug("last.fm::loading_scrobble(%p)", d);
        _debug("  scrobble::title: %s", d->title);
        _debug("  scrobble::artist: %s", d->artist);
        _debug("  scrobble::album: %s", d->album);
        _debug("  scrobble::length: %lu", d->length);
        _debug("  scrobble::position: %.2f", d->position);
        _debug("  scrobble::scrobbled: %s", d->scrobbled ? "yes" : "no");
        _debug("  scrobble::track_number: %u", d->track_number);
        _debug("  scrobble::start_time: %lu", d->start_time);
        _debug("  scrobble::play_time: %.2f", d->play_time);
    }
}

void lastfm_scrobble(struct scrobbler *s, const scrobble track)
{
    if (NULL == s) { return; }
    _info("last.fm::scrobble %s//%s//%s", track.title, track.artist, track.album);
}

void lastfm_now_playing(struct scrobbler *s, const scrobble *track)
{
    if (NULL == s) { return; }
    if (NULL == track) { return; }

    if (!now_playing_is_valid(track)) {
        _warn("last.fm::invalid_now_playing: %s//%s//%s", track->title, track->artist, track->album);
        return;
    }

    _info("last.fm::now_playing: %s//%s//%s", track->title, track->artist, track->album);

    for (size_t i = 0; i < s->credentials_length; i++) {
        struct api_credentials *cur = s->credentials[i];
        _trace("api::submit_to[%s]", get_api_type_label(cur->end_point));
        if (s->credentials[i]->enabled) {
            http_request *req = api_build_request_now_playing(track, s->curl, cur->end_point);
            http_response *res = http_response_new();

            api_post_request(s->curl, req, res);

            http_request_free(req);
            http_response_free(res);
        }
    }
}

size_t scrobbles_consume_queue(struct mpris_player *player, struct scrobbler *scrobbler)
{
    size_t consumed = 0;
    size_t queue_length = player->queue_length;
    _trace("last.fm::queue_length: %i", queue_length);
    if (queue_length > 0) {
        for (size_t pos = 0; pos < queue_length; pos++) {
             scrobble *current = player->queue[pos];

            if (scrobble_is_valid(current)) {
                _trace("last.fm::scrobble_pos(%p//%i): valid", current, pos);
                lastfm_scrobble(scrobbler, *current);
                current->scrobbled = true;
                if (pos > 0) {
                    scrobbles_remove(player, pos);
                }
                consumed++;
            } else {
                if (!current->scrobbled) {
                    _warn("last.fm::invalid_scrobble:pos(%p//%i) %s//%s//%s", current, pos, current->title, current->artist, current->album);
                }
                scrobbles_remove(player, pos);
            }
        }
    }
    if (consumed > 0) {
        _trace("last.fm::queue_consumed: %i", consumed);
    }
    return consumed;
}

bool scrobbles_has_previous(const struct mpris_player *player)
{
    if(NULL != player) { return false; }
    return (NULL != player->previous);
}
#endif // SCROBBLE_H
