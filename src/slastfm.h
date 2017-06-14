/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef SLASTFM_H
#define SLASTFM_H

#include <lastfmlib/lastfmscrobblerc.h>
#include <time.h>

#define API_KEY "990909c4e451d6c1ee3df4f5ee87e6f4"
#define API_SECRET "8bde8774564ef206edd9ef9722742a72"
#define LASTFM_NOW_PLAYING_DELAY 65 //seconds
#define LASTFM_MIN_TRACK_LENGTH 30 // seconds

int _log(log_level level, const char* format, ...);

#if 0
const char* get_lastfm_status_label (lastfm_call_status status)
{
    switch (status) {
        case ok:
            return "ok";
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

lastfm_scrobbler* lastfm_create_scrobbler(char* user_name, char* password)
{
    lastfm_scrobbler *s = create_scrobbler(user_name, password, 0, 1);

    if (NULL == s) {
        _log(error, "last.fm::login: failed");
        return NULL;
    } else {
        _log(debug, "last.fm::login: ok", "ok");
    }
    return s;
}

void lastfm_destroy_scrobbler(lastfm_scrobbler *s)
{
    if (NULL == s) { return; }
    destroy_scrobbler(s);
}

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

    _log(tracing, "mem::inited_scrobble(%p)", s);
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
        _log(tracing, "mem::scrobble::freeing_title(%p): %s", s->title, s->title);
        free(s->title);
        s->title = NULL;
    }
    if (NULL != s->album) {
        _log(tracing, "mem::scrobble::freeing_album(%p): %s", s->album, s->album);
        free(s->album);
        s->album = NULL;
    }
    if (NULL != s->artist) {
        _log(tracing, "mem::scrobble:freeing_artist(%p): %s", s->artist, s->artist);
        free(s->artist);
        s->artist = NULL;
    }

    _log (tracing, "mem::freeing_scrobble(%p)", s);
    free(s);
}

void events_free(events *);
void dbus_close(state *);
void state_free(state *s)
{
    _log(tracing, "mem::freeing_state(%p)::queue_length:%u", s, s->queue_length);
    for (unsigned i = 0; i < s->queue_length; i++) {
        scrobble_free(s->queue[i]);
    }
    if (NULL != s->properties) { mpris_properties_unref(s->properties); }
    if (NULL != s->dbus) { dbus_close(s); }
    if (NULL != s->events) { events_free(s->events); }

#if 0
    lastfm_destroy_scrobbler(s->scrobbler);
#endif
    free(s);
}

events* events_new();
dbus *dbus_connection_init(state*);
void get_mpris_properties(DBusConnection*, const char*, mpris_properties*);
char *get_player_namespace(DBusConnection*);
void state_loaded_properties(state* , mpris_properties*);
void state_init(state *s)
{
    extern lastfm_credentials credentials;
    s->queue_length = 0;
    s->player_state = stopped;

    _log(tracing, "mem::initing_state(%p)", s);
#if 0
    s->scrobbler = lastfm_create_scrobbler(credentials.user_name, credentials.password);
#endif
    s->properties = mpris_properties_new();
    s->events = events_new();
    s->dbus = dbus_connection_init(s);

    char* destination = get_player_namespace(s->dbus->conn);
    if (NULL != destination ) {
        mpris_properties *properties = mpris_properties_new();

        get_mpris_properties(s->dbus->conn, destination, properties);
        state_loaded_properties(s, properties);

        mpris_properties_unref(properties);
    }
    _log(tracing, "mem::inited_state(%p)", s);
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
    _log(tracing, "Checking playtime: %u - %u", s->play_time, (s->length / 2));
    _log(tracing, "Checking scrobble:\n" \
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

void scrobbles_append(state *s, const scrobble *m)
{
#if 0
    if (m->play_time <= (m->length / 2.0f)) {
        _log(tracing, "last.fm::not_enough_play_time: %.2f < %.2f", m->play_time, m->length/2.0f);
        return;
    }
#endif
    scrobble *n = scrobble_new();
    scrobble_copy(n, m);

    if (s->queue_length > QUEUE_MAX_LENGTH) {
        _log(error, "last.fm::queue_length_exceeded: please check connection");
        scrobble_free(n);
        return;
    }
    _log (debug, "last.fm::appending_scrobble(%p//%u) %s//%s//%s", n, s->queue_length, n->title, n->artist, n->album);
    s->queue[s->queue_length++] = n;
}

void scrobbles_remove(state *s, size_t pos)
{
    scrobble *last = s->queue[pos];
    _log (debug, "last.fm::popping_scrobble(%p//%u) %s//%s//%s", s->queue[pos], pos, last->title, last->artist, last->album);
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
    _log(tracing, "last.fm::peeking_at:%d: (%p)", i, s->queue[i]);
    if (i <= s->queue_length && NULL != s->queue[i]) {
        return s->queue[i];
    }
    return NULL;
}

playback_state get_mpris_playback_status(const mpris_properties *p)
{
    playback_state state = stopped;
    if (NULL != p->playback_status) {
        if (mpris_properties_is_playing(p)) {
            state = playing;
        }
        if (mpris_properties_is_paused(p)) {
            state = paused;
        }
        if (mpris_properties_is_stopped(p)) {
            state = stopped;
        }
    }
    return state;
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
        e->track_changed = true;
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
        _log(tracing, "last.fm::checking_volume_changed:\t\t%3s %.2f -> %.2f",
             e->volume_changed ? "yes" : "no",
             last->volume, p->volume);
    } else {
        _log(info, "last.fm::checking_volume_changed:\t\t%3s", e->volume_changed ? "yes" : "no");
    }
    if (last && p) {
        _log(tracing, "last.fm::checking_playback_status_changed:\t%3s %s -> %s",
             e->playback_status_changed ? "yes" : "no",
             last->playback_status, p->playback_status);
    } else {
        _log(info, "last.fm::checking_playback_status_changed:\t%3s", e->playback_status_changed ? "yes" : "no");
    }
    if (last && p) {
        _log(tracing, "last.fm::checking_track_changed:\t\t%3s %s -> %s",
            e->track_changed ? "yes" : "no",
            last->metadata->title, p->metadata->title);
    } else {
        _log(info, "last.fm::checking_track_changed:\t\t%3s", e->track_changed ? "yes" : "no");
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
    _log(debug, "last.fm::loading_scrobble(%p)", d);

    if (NULL != d->title) { free(d->title); }
    cpstr(&d->title, p->metadata->title, MAX_PROPERTY_LENGTH);
    if (NULL != d->album) { free(d->album); }
    cpstr(&d->album, p->metadata->album, MAX_PROPERTY_LENGTH);
    if (NULL != d->artist) { free(d->artist); }
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
    _log(debug, "  scrobble::title: %s", d->title);
    _log(debug, "  scrobble::artist: %s", d->artist);
    _log(debug, "  scrobble::album: %s", d->album);
    _log(debug, "  scrobble::length: %lu", d->length);
    _log(debug, "  scrobble::position: %.2f", d->position);
    _log(debug, "  scrobble::scrobbled: %s", d->scrobbled ? "yes" : "no");
    _log(debug, "  scrobble::track_number: %u", d->track_number);
    _log(debug, "  scrobble::start_time: %lu", d->start_time);
    _log(debug, "  scrobble::play_time: %.2f", d->play_time);
}

void lastfm_scrobble(lastfm_scrobbler *s, const scrobble track)
{
    if (NULL == s) {
        return;
    } else {
        _log(info, "last.fm::scrobble %s//%s//%s", track.title, track.artist, track.album);
#if 0
        finished_playing(s);
#endif
    }
}

void lastfm_now_playing(lastfm_scrobbler *s, const scrobble *track)
{
    if (NULL == s) { return; }

#if 0
    submission_info *scrobble = create_submission_info();
    if (NULL == scrobble) { return; }
    if (NULL == scrobble->track) { return; }
    if (NULL == scrobble->album) { return; }
    if (NULL == scrobble->artist) { return; }
#endif
    if (!now_playing_is_valid(track)) {
        _log(warning, "last.fm::invalid_now_playing: %s//%s//%s", track->title, track->artist, track->album);
        return;
    }

#if 0
    scrobble->track = track->title;
    scrobble->artist = track->artist;
    scrobble->album = track->album;
    scrobble->track_length_in_secs = track->length;
    scrobble->time_started = track->start_time;

    started_playing (s, scrobble);
#endif

    _log(info, "last.fm::now_playing: %s//%s//%s", track->title, track->artist, track->album);
}

size_t scrobbles_consume_queue(state *s)
{
    size_t consumed = 0;
    size_t queue_length = s->queue_length;
    _log(tracing, "last.fm::queue_length: %i", queue_length);
    if (queue_length > 0) {
        for (size_t pos = 0; pos < queue_length; pos++) {
             scrobble *current = s->queue[pos];
//             scrobble *current = scrobbles_pop(s);

            if (scrobble_is_valid(current)) {
                _log(debug, "last.fm::scrobble_pos(%p//%i)", current, pos);
                lastfm_scrobble(s->scrobbler, *current);
                current->scrobbled = true;
                scrobbles_remove(s, pos);
                consumed++;
            } else {
                _log(warning, "last.fm::invalid_scrobble:pos(%p//%i) %s//%s//%s", current, pos, current->title, current->artist, current->album);
            }
        }
    }
    if (consumed > 0) {
        _log(tracing, "last.fm::queue_consumed: %i", consumed);
    }
    return consumed;
}

bool scrobbles_has_previous(const state *s)
{
    if(NULL != s) { return false; }
    return (NULL != s->previous);
}
#endif // SLASTFM_H
