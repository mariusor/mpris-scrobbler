/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
//#include <clastfm.h>
#include <lastfmlib/lastfmscrobblerc.h>
#include <time.h>

#define API_KEY "990909c4e451d6c1ee3df4f5ee87e6f4"
#define API_SECRET "8bde8774564ef206edd9ef9722742a72"
#define LASTFM_NOW_PLAYING_DELAY 65
#define QUEUE_MAX_LENGTH 20 // scrobbles
#define LASTFM_MIN_TRACK_LENGTH 30 // seconds

int _log(log_level level, const char* format, ...);

typedef enum lastfm_call_statuses {
    ok = 0,
    unavaliable = 1, //The service is temporarily unavailable, please try again.
    invalid_service = 2, //Invalid service - This service does not exist
    invalid_method = 3, //Invalid Method - No method with that name in this package
    authentication_failed = 4, //Authentication Failed - You do not have permissions to access the service
    invalid_format = 5, //Invalid format - This service doesn't exist in that format
    invalid_parameters = 6, //Invalid parameters - Your request is missing a required parameter
    invalid_resource = 7, //Invalid resource specified
    operation_failed = 8, //Operation failed - Something else went wrong
    invalid_session_key = 9, //Invalid session key - Please re-authenticate
    invalid_apy_key = 10, //Invalid API key - You must be granted a valid key by last.fm
    service_offline = 11, //Service Offline - This service is temporarily offline. Try again later.
    invalid_signature = 13, //Invalid method signature supplied
    temporary_error = 16, //There was a temporary error processing your request. Please try again
    suspended_api_key = 26, //Suspended API key - Access for your account has been suspended, please contact Last.fm
    rate_limit_exceeded = 29 //Rate limit exceeded - Your IP has made too many requests in a short period
} lastfm_call_status;

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

typedef struct scrobble {
    char* title;
    char* album;
    char* artist;
    bool scrobbled;
    unsigned short track_number;
    unsigned length;
    time_t start_time;
    unsigned play_time;
    unsigned position;
} scrobble;

void scrobble_init(scrobble *s)
{
    if (NULL == s) { return; }
    s->title = NULL;
    s->album = NULL;
    s->artist = NULL;
    s->scrobbled = false;
    s->length = 0;
    s->position = 0;
    s->start_time = 0;
    s->track_number = 0;

    _log(tracing, "mem::inited_scrobble:%p", s);
}

void scrobble_free(scrobble *s)
{
    if (NULL == s) return;

    if (NULL != s->title) { free(s->title); }
    if (NULL != s->album) { free(s->album); }
    if (NULL != s->artist) { free(s->artist); }

    _log (tracing, "mem::freed_scrobble(%p)", s);
    free(s);
}

typedef enum playback_states {
    stopped = 1 << 0,
    paused = 1 << 1,
    playing = 1 << 2
} playback_state;

typedef struct session_scrobbles {
    playback_state player_state;
    time_t *start_time;
    size_t queue_length;
    scrobble *current;
    scrobble *previous;
    scrobble *queue[QUEUE_MAX_LENGTH];
    lastfm_scrobbler *scrobbler;
} scrobbles;

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
#define EMPTY_SCROBBLES { .player_state=stopped, .start_time=NULL, .queue_length=0, .current=NULL, .previous=NULL, .queue={ NULL } }

void scrobbles_init(scrobbles *s)
{
    s->queue_length = 0;
    time(s->start_time);
    s->current = (scrobble*)calloc(1, sizeof(scrobble));
    scrobble_init(s->current);
    _log(tracing, "mem::inited_scrobbles:%p", s);
}

void scrobble_copy (scrobble *t, const scrobble *s)
{
    if (NULL == t) { return; }

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

void scrobbles_append(scrobbles *s, const scrobble *m)
{
    scrobble *n = (scrobble*)calloc(1, sizeof(scrobble));
    scrobble_init(n);
    scrobble_copy(n, m);

    if (s->queue_length > QUEUE_MAX_LENGTH) {
        _log(error, "last.fm::queue_length_exceeded: please check connection");
        scrobble_free(n);
        return;
    }
    _log (debug, "last.fm::appended_scrobble(%p//%u) %s//%s//%s", n, s->queue_length, n->title, n->artist, n->album);
    s->queue[s->queue_length++] = n;

    s->previous = n;
}

void lastfm_destroy_scrobbler(lastfm_scrobbler *s)
{
    if (NULL == s) { return; }
    destroy_scrobbler(s);
}

void scrobbles_free(scrobbles *s)
{
    for (unsigned i = 0; i < s->queue_length; i++) {
        scrobble_free(s->queue[i]);
    }
    if (NULL != s->previous) { scrobble_free(s->previous); }
    if (NULL != s->previous) { scrobble_free(s->current); }
    lastfm_destroy_scrobbler(s->scrobbler);
    _log(tracing, "mem::freed_scrobbles(%p)", s);
}

void scrobbles_remove(scrobbles *s, size_t pos)
{
    scrobble *last = s->queue[pos];
    _log (debug, "last.fm::popping_scrobble(%p//%u) %s//%s//%s", s->queue[pos], pos, last->title, last->artist, last->album);
    scrobble_free(s->queue[pos]);

    s->queue_length--;
    s->previous = s->queue[pos-1];
}

scrobble* scrobbles_pop(scrobbles *s)
{
    scrobble *last = (scrobble*)calloc(1, sizeof(scrobble));
    scrobble_init(last);

    size_t cnt = s->queue_length - 1;
    scrobble_copy(last, s->queue[cnt]);
    scrobbles_remove(s, cnt);

    return last;
}

scrobble* scrobbles_peek_queue(scrobbles *s, size_t i)
{
    _log(tracing, "last.fm::peeking_at:%d: (%p)", i, s->queue[i]);
    if (i <= s->queue_length && NULL != s->queue[i]) {
        return s->queue[i];
    }
    return NULL;
}


void load_scrobble(const mpris_properties *p, scrobble* s)
{
    if (NULL == s) { return; }
    if (NULL == p) { return; }

    time_t tstamp = time(0);
    _log(tracing, "load_scrobble::%p", s);

    scrobble_init(s);

    //if (NULL == p->metadata.title) { return; }
    size_t t_len = strlen(p->metadata.title);
    s->title = (char*)calloc(1, t_len+1);

    if (NULL == s->title) { free(s); return; }

    strncpy(s->title, p->metadata.title, t_len);

    //if (NULL == p->metadata.album) { return; }
    size_t al_len = strlen(p->metadata.album);
    s->album = (char*)calloc(1, al_len+1);

    if (NULL == s->album) { scrobble_free(s); return; }

    strncpy(s->album, p->metadata.album, al_len);

    //if (NULL == p->metadata.artist) { return; }
    size_t ar_len = strlen(p->metadata.artist);
    s->artist = (char*)calloc(1, ar_len+1);

    if (NULL == s->artist) { scrobble_free(s); return; }

    strncpy(s->artist, p->metadata.artist, ar_len);

    if (p->metadata.length > 0) {
        s->length = p->metadata.length / 1000000;
    } else {
        s->length = 60;
    }
    if (p->position > 0) {
        s->position = p->position / 1000000;
    }
    s->scrobbled = false;
    s->track_number = p->metadata.track_number;
    s->start_time = tstamp;
    s->play_time = 0;
}

bool scrobble_is_valid(const scrobble *s)
{
    if (NULL == s) { return false; }
    if (NULL == s->title) { return false; }
    if (NULL == s->album) { return false; }
    if (NULL == s->artist) { return false; }
    if (s->scrobbled) { return false; }
    //_log(tracing, "Checking playtime: %u - %u", s->play_time, (s->length / 2));
#if 0 
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

bool now_playing_is_valid(const scrobble *m) {
    return (
        NULL != m &&
        NULL != m->title && strlen(m->title) > 0 &&
        NULL != m->artist && strlen(m->artist) > 0 &&
        NULL != m->album && strlen(m->album) > 0 &&
        m->length > 0
    );
}

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

void lastfm_scrobble(lastfm_scrobbler *s, const scrobble track)
{
    if (NULL == s) { return; }

#ifndef DEBUG
    finished_playing(s);
#endif
    _log(info, "last.fm::scrobble %s//%s//%s", track.title, track.artist, track.album);
}

void lastfm_now_playing(lastfm_scrobbler *s, const scrobble track)
{
    if (NULL == s) { return; }

#ifndef DEBUG
    submission_info *scrobble = create_submission_info();
    if (NULL == scrobble) { return; }

    scrobble->track = track.title;
    scrobble->artist = track.artist;
    scrobble->album = track.album;
    scrobble->track_length_in_secs = track.length;
    scrobble->time_started = track.start_time;

    started_playing (s, scrobble);
#endif

    _log(info, "last.fm::now_playing: %s//%s//%s", track.title, track.artist, track.album);
}

size_t scrobbles_consume_queue(scrobbles *s)
{
    size_t consumed = 0;
    size_t queue_length = s->queue_length;
    if (queue_length > 0) {
#if 0
        _log(tracing, "last.fm::queue_length: %i", queue_length);
#endif
        for (size_t pos = 0; pos < queue_length; pos++) {
//             scrobble *current = s->queue[pos];
             scrobble *current = scrobbles_pop(s);

            if (scrobble_is_valid(current)) {
                _log(info, "last.fm::scrobble_pos(%p//%i)", current, pos);
                lastfm_scrobble(s->scrobbler, *current);
                current->scrobbled = true;
                scrobbles_remove(s, pos);
                consumed++;
            }
        }
    }
    if (consumed) {
        _log(tracing, "last.fm::queue_consumed: %i", consumed);
    }
    return consumed;
}

bool scrobbles_has_track_changed(const scrobbles *s, const scrobble *new_track)
{
   bool result = false;
   if (NULL == s) { return false; }
   if (NULL == s->current) { return false; }
   if (NULL == new_track) { return false; }
   if (NULL == s->current->title) { return false; }
   if (NULL == s->current->artist) { return false; }
   if (NULL == s->current->album) { return false; }
   if (NULL == new_track->title) { return false; }
   if (NULL == new_track->artist) { return false; }
   if (NULL == new_track->album) { return false; }

   result = strncmp(s->current->title, new_track->title, strlen(s->current->title)) &&
            strncmp(s->current->album, new_track->album, strlen(s->current->album)) &&
            strncmp(s->current->artist, new_track->artist, strlen(s->current->artist));

   return result;
}

bool scrobbles_has_previous(const scrobbles *s)
{
    if(NULL != s) { return false; }
    return (NULL != s->previous);
}

inline bool scrobbles_is_playing(const scrobbles *s)
{
    return (s->player_state == playing);
}

inline bool scrobbles_is_paused(const scrobbles *s)
{
    return (s->player_state == paused);
}

inline bool scrobbles_is_stopped(const scrobbles *s)
{
    return (s->player_state == stopped);
}
