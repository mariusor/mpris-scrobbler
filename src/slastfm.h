/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef SLASTFM_H
#define SLASTFM_H

//#include <lastfmlib/lastfmscrobblerc.h>
#include <time.h>

#define LASTFM_API_KEY "990909c4e451d6c1ee3df4f5ee87e6f4"
#define LASTFM_API_SECRET "8bde8774564ef206edd9ef9722742a72"
#define LASTFM_NOW_PLAYING_DELAY 65 //seconds
#define LASTFM_MIN_TRACK_LENGTH 30 // seconds

int _log(log_level level, const char* format, ...);


#if 0
lastfm_scrobbler* lastfm_create_scrobbler(char* user_name, char* password)
{
    //lastfm_scrobbler *s = create_scrobbler(user_name, password, 0, 1);
    char *s = NULL;


    if (NULL == s) {
        _error("last.fm::login: failed");
        return NULL;
    } else {
        _debug("last.fm::login: ok", "ok");
    }
    return s;
}

void lastfm_destroy_scrobbler(lastfm_scrobbler *s)
{
    if (NULL == s) { return; }
    destroy_scrobbler(s);
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

void scrobbler_free(scrobbler *s)
{
    if (NULL == s) { return; }
    curl_easy_cleanup(s->curl);
    free(s);
}

void events_free(events *);
void dbus_close(state *);
void state_free(state *s)
{
    _trace("mem::freeing_state(%p)::queue_length:%u", s, s->queue_length);
    for (unsigned i = 0; i < s->queue_length; i++) {
        scrobble_free(s->queue[i]);
    }
    if (NULL != s->properties) { mpris_properties_unref(s->properties); }
    if (NULL != s->dbus) { dbus_close(s); }
    if (NULL != s->events) { events_free(s->events); }
    if (NULL != s->scrobbler) { scrobbler_free(s->scrobbler); }

    free(s);
}

scrobbler *scrobbler_new()
{
    scrobbler *result = malloc(sizeof(scrobbler));

    return (result);
}

void scrobbler_init(scrobbler *s, configuration *config)
{
    s->credentials = config->credentials;
    s->credentials_length = config->credentials_length;
    s->curl = curl_easy_init();
}

events* events_new();
dbus *dbus_connection_init(state*);
void get_mpris_properties(DBusConnection*, const char*, mpris_properties*);
char *get_player_namespace(DBusConnection*);
void state_loaded_properties(state* , mpris_properties*);
void state_init(state *s)
{
    extern configuration global_config;
    s->queue_length = 0;
    s->player_state = stopped;

    _trace("mem::initing_state(%p)", s);
    s->scrobbler = scrobbler_new();

    scrobbler_init(s->scrobbler, &global_config);
    s->properties = mpris_properties_new();
    s->events = events_new();
    s->dbus = dbus_connection_init(s);

    char* destination = get_player_namespace(s->dbus->conn);
    if (NULL != destination ) {
        mpris_properties *properties = mpris_properties_new();

        get_mpris_properties(s->dbus->conn, destination, properties);
        state_loaded_properties(s, properties);

        mpris_properties_unref(properties);
        free(destination);
    }
    _trace("mem::inited_state(%p)", s);
}

state* state_new()
{
    state *result = malloc(sizeof(state));
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

void scrobbles_append(state *s, const scrobble *m)
{
#if 0
    if (m->play_time <= (m->length / 2.0f)) {
        _trace("last.fm::not_enough_play_time: %.2f < %.2f", m->play_time, m->length/2.0f);
        return;
    }
#endif
    if (s->queue_length > 0 && scrobbles_equals(s->current, m)) {
        _debug("last.fm::queue: skipping existing scrobble(%p)",m);
        return;
    }
    scrobble *n = scrobble_new();
    scrobble_copy(n, m);

    if (s->queue_length > QUEUE_MAX_LENGTH) {
        _error("last.fm::queue_length_exceeded: please check connection");
        scrobble_free(n);
        return;
    }
    for (size_t i = s->queue_length; i > 0; i--) {
        scrobble *to_move = s->queue[i-1];
        if(to_move == m) { continue; }
        s->queue[i] = to_move;
        _debug("last.fm::queue_move_scrobble(%p//%u<-%u) %s//%s//%s", to_move, i, i-1, to_move->title, to_move->artist, to_move->album);
    }
    s->queue_length++;
    _debug("last.fm::queue_push_scrobble(%p//%4u) %s//%s//%s", n, 0, n->title, n->artist, n->album);
    s->queue[0] = n;
}

void scrobbles_remove(state *s, size_t pos)
{
    scrobble *last = s->queue[pos];
    _debug("last.fm::popping_scrobble(%p//%u) %s//%s//%s", s->queue[pos], pos, last->title, last->artist, last->album);
    if (pos >= s->queue_length) { return; }

    scrobble_free(s->queue[pos]);

    s->queue_length--;
    s->previous = s->queue[pos-1];
}

scrobble* scrobbles_pop(state *s)
{
    scrobble *last = scrobble_new();

    size_t cnt = s->queue_length - 1;
    if (NULL == s->queue[cnt]) { return NULL; }
    scrobble_copy(last, s->queue[cnt]);
    scrobbles_remove(s, cnt);

    return last;
}

scrobble* scrobbles_peek_queue(state *s, size_t i)
{
    _trace("last.fm::peeking_at:%d: (%p)", i, s->queue[i]);
    if (i <= s->queue_length && NULL != s->queue[i]) {
        return s->queue[i];
    }
    return NULL;
}

void load_event(mpris_event* e, const mpris_properties *p, const state *state)
{
    if (NULL == e) { goto _return; }
    if (NULL == p) { goto _return; }
    if (NULL == p->metadata) { goto _return; }
    e->player_state = get_mpris_playback_status(p);
    e->track_changed = false;
    e->volume_changed = false;
    e->playback_status_changed = false;

    if (NULL == state) { goto _return; }
    e->playback_status_changed = (e->player_state != state->player_state);

    mpris_properties *last = state->properties;
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

void lastfm_scrobble(scrobbler *s, const scrobble track)
{
    if (NULL == s) { return; }
    _info("last.fm::scrobble %s//%s//%s", track.title, track.artist, track.album);
}

void lastfm_now_playing(scrobbler *s, const scrobble *track)
{
    if (NULL == s) { return; }
    if (NULL == track) { return; }

    if (!now_playing_is_valid(track)) {
        _warn("last.fm::invalid_now_playing: %s//%s//%s", track->title, track->artist, track->album);
        return;
    }

    _info("last.fm::now_playing: %s//%s//%s", track->title, track->artist, track->album);

    for (size_t i = 0; i < s->credentials_length; i++) {
        api_credentials *cur = s->credentials[i];
        _trace("api::submit_to[%s]", get_api_type_label(cur->end_point));
        api_request *req = api_build_request_now_playing(track, s->curl, cur->end_point);
        api_response *res = api_response_new();

        api_post_request(s->curl, req, res);

        api_request_free(req);
        api_response_free(res);
    }
}

size_t scrobbles_consume_queue(state *s)
{
    size_t consumed = 0;
    size_t queue_length = s->queue_length;
    _trace("last.fm::queue_length: %i", queue_length);
    if (queue_length > 0) {
        for (size_t pos = 0; pos < queue_length; pos++) {
             scrobble *current = s->queue[pos];

            if (scrobble_is_valid(current)) {
                _trace("last.fm::scrobble_pos(%p//%i): valid", current, pos);
                lastfm_scrobble(s->scrobbler, *current);
                current->scrobbled = true;
                if (pos > 0) {
                    scrobbles_remove(s, pos);
                }
                consumed++;
            } else {
                if (!current->scrobbled) {
                    _warn("last.fm::invalid_scrobble:pos(%p//%i) %s//%s//%s", current, pos, current->title, current->artist, current->album);
                }
                scrobbles_remove(s, pos);
            }
        }
    }
    if (consumed > 0) {
        _trace("last.fm::queue_consumed: %i", consumed);
    }
    return consumed;
}

bool scrobbles_has_previous(const state *s)
{
    if(NULL != s) { return false; }
    return (NULL != s->previous);
}
#endif // SLASTFM_H
