/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef SLASTFM_H
#define SLASTFM_H

#include <lastfmlib/lastfmscrobblerc.h>
#include <time.h>

#include "sdbus.h"

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

void scrobble_free(scrobble *s)
{
    if (NULL == s) return;

    if (NULL != s->title) { free(s->title); }
    if (NULL != s->album) { free(s->album); }
    if (NULL != s->artist) { free(s->artist); }

    _log (tracing, "mem::freeing_scrobble(%p)", s);
    free(s);
}

void events_free(events *);
void state_free(state *s)
{
    _log(tracing, "mem::freeing_state(%p)::queue_length:%u", s, s->queue_length);
    for (unsigned i = 0; i < s->queue_length; i++) {
        scrobble_free(s->queue[i]);
    }
    if (NULL != s->current) { scrobble_free(s->current); }
    if (NULL != s->properties) { mpris_properties_unref(s->properties); }
    if (NULL != s->dbus) { dbus_close(s); }
    if (NULL != s->events) { events_free(s->events); }

    lastfm_destroy_scrobbler(s->scrobbler);
    free(s);
}

events* events_new();
void state_init(state *s)
{
    extern lastfm_credentials credentials;
    s->queue_length = 0;
    s->playback_status = stopped;
    s->scrobbler = lastfm_create_scrobbler(credentials.user_name, credentials.password);

    s->current = malloc(sizeof(scrobble));
    scrobble_init(s->current);

    s->properties = malloc(sizeof(mpris_properties));
    mpris_properties_init(s->properties);

    s->events = events_new();
    s->dbus = dbus_connection_init(s);

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
        strlen(s->title) > 0 &&
        strlen(s->artist) > 0 &&
        strlen(s->album) > 0 &&
        s->length >= LASTFM_MIN_TRACK_LENGTH &&
        s->play_time >= (s->length / 2)
    );
}

bool now_playing_is_valid(const scrobble *m, const time_t current_time, const time_t last_playing_time) {
    return (
        NULL != m &&
        NULL != m->title && strlen(m->title) > 0 &&
        NULL != m->artist && strlen(m->artist) > 0 &&
        NULL != m->album && strlen(m->album) > 0 &&
        m->length > 0 &&
        last_playing_time > 0 &&
        difftime(current_time, last_playing_time) >= LASTFM_NOW_PLAYING_DELAY
    );
}

void scrobble_copy (scrobble *t, const scrobble *s)
{
    if (NULL == t) { return; }
    if (NULL == s) { return; }
    if (NULL == s->title) { return; }
    if (NULL == s->artist) { return; }
    if (NULL == s->album) { return; }

    size_t t_len = strlen(s->title);
    t->title = (char*)calloc(1, t_len+1);

    if (NULL == t->title) { free(t); return; }

    strncpy(t->title, s->title, t_len);

    size_t al_len = strlen(s->album);
    t->album = (char*)calloc(1, al_len+1);

    if (NULL == t->album) { scrobble_free(t); return; }

    strncpy(t->album, s->album, al_len);

    size_t ar_len = strlen(s->artist);
    t->artist = (char*)calloc(1, ar_len+1);

    if (NULL == t->artist) { scrobble_free(t); return; }

    strncpy(t->artist, s->artist, ar_len);

    t->length = s->length;
    t->position = s->position;
    t->start_time = s->start_time;
    t->play_time = s->play_time;
    t->track_number = s->track_number;
}

void scrobbles_append(state *s, const scrobble *m)
{
    if (m->play_time <= (m->length / 2.0f)) {
        _log(tracing, "last.fm::not_enough_play_time: %.2f < %.2f", m->play_time, m->length/2.0f);
        return;
    }
    scrobble *n = (scrobble*)malloc(sizeof(scrobble));
    scrobble_init(n);
    scrobble_copy(n, m);

    if (s->queue_length > QUEUE_MAX_LENGTH) {
        _log(error, "last.fm::queue_length_exceeded: please check connection");
        scrobble_free(n);
        return;
    }
    _log (debug, "last.fm::appending_scrobble(%p//%u) %s//%s//%s::%.2f", n, s->queue_length, n->title, n->artist, n->album, n->play_time);
    s->queue[s->queue_length++] = n;

    s->previous = n;
}

void scrobbles_remove(state *s, size_t pos)
{
    scrobble *last = s->queue[pos];
    _log (debug, "last.fm::popping_scrobble(%p//%u) %s//%s//%s::%.2f", s->queue[pos], pos, last->title, last->artist, last->album, last->play_time);
    if (pos >= s->queue_length) { return; }

    scrobble_free(s->queue[pos]);

    s->queue_length--;
    s->previous = s->queue[pos-1];
}

scrobble* scrobbles_pop(state *s)
{
    scrobble *last = (scrobble*)calloc(1, sizeof(scrobble));
    scrobble_init(last);

    size_t cnt = s->queue_length - 1;
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
        if (strncmp(p->playback_status, MPRIS_PLAYBACK_STATUS_PLAYING, strlen(MPRIS_PLAYBACK_STATUS_PLAYING)) == 0) {
            state = playing;
        }
        if (strncmp(p->playback_status, MPRIS_PLAYBACK_STATUS_PAUSED, strlen(MPRIS_PLAYBACK_STATUS_PAUSED)) == 0) {
            state = paused;
        }
    }
    return state;
}

void load_event(const mpris_properties *p, const state *state, mpris_event* e)
{

    if (NULL == e) { goto _return; }
    if (NULL == p) { goto _return; }
    if (NULL == p->metadata) { goto _return; }
    e->track_changed = false;
    e->player_state = stopped;
    e->volume_changed = false;
    e->playback_status_changed = false;

    if (NULL == state) { goto _return; }

    e->player_state = get_mpris_playback_status(p);
    if (e->player_state != state->playback_status) { e->playback_status_changed = true; }

    mpris_properties *last = state->properties;
    if (NULL == last) { goto _return; }
    if (NULL == last->metadata) { goto _return; }

    if (last->volume != p->volume) {
        e->volume_changed = true;
    }
    if (
        (NULL != p->metadata->title) &&
        (NULL != p->metadata->album) &&
        (NULL != p->metadata->artist) &&
        (NULL != last->metadata->title) &&
        (NULL != last->metadata->album) &&
        (NULL != last->metadata->artist)
    ) {
        e->track_changed = strncmp(p->metadata->title, last->metadata->title, strlen(p->metadata->title)) &&
            strncmp(p->metadata->album, last->metadata->album, strlen(p->metadata->album)) &&
            strncmp(p->metadata->artist, last->metadata->artist, strlen(p->metadata->artist));
    }
_return:
    _log(info, "last.fm::checking_volume_changed: %s", e->volume_changed ? "yes" : "no");
//    _log(tracing, "\t\t%.2f -> %.2f", last->volume, p->volume);
    _log(info, "last.fm::checking_playback_status_changed: %s", e->playback_status_changed ? "yes" : "no");
//    _log(tracing, "\t\t%s -> %s", last->playback_status, p->playback_status);
    _log(info, "last.fm::checking_track_changed: %s", e->track_changed ? "yes" : "no");
}

void load_scrobble(const mpris_properties *p, scrobble* s)
{
    if (NULL == s) { return; }
    if (NULL == p) { return; }

    time_t tstamp = time(0);
    _log(debug, "last.fm::loading_scrobble(%p)", s);

    size_t t_len = strlen(p->metadata->title);
    s->title = (char*)calloc(1, t_len+1);

    if (NULL == s->title) { free(s); return; }

    strncpy(s->title, p->metadata->title, t_len);

    size_t al_len = strlen(p->metadata->album);
    s->album = (char*)calloc(1, al_len+1);

    if (NULL == s->album) { scrobble_free(s); return; }

    strncpy(s->album, p->metadata->album, al_len);

    size_t ar_len = strlen(p->metadata->artist);
    s->artist = (char*)calloc(1, ar_len+1);

    if (NULL == s->artist) { scrobble_free(s); return; }

    strncpy(s->artist, p->metadata->artist, ar_len);

    if (p->metadata->length > 0) {
        s->length = p->metadata->length / 1000000.0f;
    } else {
        s->length = 60;
    }
    if (p->position > 0) {
        s->position = p->position / 1000000.0f;
    }
    s->scrobbled = false;
    s->track_number = p->metadata->track_number;
    s->start_time = tstamp;
    s->play_time = s->position;
    _log(debug, "  scrobble::title: %s", s->title);
    _log(debug, "  scrobble::artist: %s", s->artist);
    _log(debug, "  scrobble::album: %s", s->album);
    _log(debug, "  scrobble::length: %lu", s->length);
    _log(debug, "  scrobble::position: %.2f", s->position);
    _log(debug, "  scrobble::scrobbled: %s", s->scrobbled ? "yes" : "no");
    _log(debug, "  scrobble::track_number: %u", s->track_number);
    _log(debug, "  scrobble::start_time: %lu", s->start_time);
    _log(debug, "  scrobble::play_time: %.2f", s->play_time);
}

void lastfm_scrobble(lastfm_scrobbler *s, const scrobble track)
{
    if (NULL == s) {
        _log(warning, "last.fm::scrobble: %s//%s//%s", track.title, track.artist, track.album);
    } else {
        finished_playing(s);
        _log(info, "last.fm::scrobble %s//%s//%s", track.title, track.artist, track.album);
    }
}

void lastfm_now_playing(lastfm_scrobbler *s, const scrobble *track)
{
    if (NULL == s) { return; }

    submission_info *scrobble = create_submission_info();
    if (NULL == scrobble) { return; }

    scrobble->track = track->title;
    scrobble->artist = track->artist;
    scrobble->album = track->album;
    scrobble->track_length_in_secs = track->length;
    scrobble->time_started = track->start_time;

    started_playing (s, scrobble);

    _log(info, "last.fm::now_playing: %s//%s//%s", track->title, track->artist, track->album);
}

size_t scrobbles_consume_queue(state *s)
{
    size_t consumed = 0;
    size_t queue_length = s->queue_length;
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
