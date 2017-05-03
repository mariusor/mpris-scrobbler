/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
//#include <clastfm.h>
#include <lastfmlib/lastfmscrobblerc.h>
#include <time.h>

#define API_KEY "990909c4e451d6c1ee3df4f5ee87e6f4"
#define API_SECRET "8bde8774564ef206edd9ef9722742a72"

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
    unsigned track_number;
    unsigned length;
    time_t start_time;
    time_t end_time;
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

    //_log(tracing, "mem::inited_scrobble:%p", s);
    //return s;
}

void scrobble_free(scrobble *s)
{
    if (NULL == s) return;

    if (NULL != s->title) free(s->title);
    if (NULL != s->album) free(s->album);
    if (NULL != s->artist) free(s->artist);

    _log (tracing, "mem::freed_scrobble(%p)", s);
    //free(s);
}

typedef struct session_scrobbles {
    time_t *start_time;
    size_t queue_length;
    scrobble *previous;
    scrobble *queue[QUEUE_MAX_LENGTH];
} scrobbles;

void scrobbles_init(scrobbles *s)
{
    s->queue_length = 0;
    time(s->start_time);
}

void scrobble_copy (scrobble *t, const scrobble *s)
{
    if (NULL == t) { return; }
    scrobble_init(t);
    size_t t_len = strlen(s->title);
    t->title = (char*)calloc(1, t_len+1);
    if (NULL == t->title) {
        free(t);
        return;
    }
    strncpy(t->title, s->title, t_len);

    size_t al_len = strlen(s->album);
    t->album = (char*)calloc(1, al_len+1);
    if (NULL == t->album) {
        scrobble_free(t);
        return;
    }
    strncpy(t->album, s->album, al_len);

    size_t ar_len = strlen(s->artist);
    t->artist = (char*)calloc(1, ar_len+1);
    if (NULL == t->artist) {
        scrobble_free(t);
        return;
    }
    strncpy(t->artist, s->artist, ar_len);

    t->length = s->length;
    t->position = s->position;
    t->start_time = s->start_time;
    t->end_time = s->end_time;
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

    if (NULL != s->previous) {
        s->previous->end_time = m->start_time;
    }
    s->previous = n;
}

void scrobbles_free(scrobbles *s)
{
    for (unsigned i = 0; i < s->queue_length; i++) {
        scrobble_free(s->queue[i]);
    }
}

scrobble* scrobbles_pop(scrobbles *s)
{
    scrobble *last = (scrobble*)calloc(1, sizeof(scrobble));
    scrobble_init(last);

    size_t cnt = s->queue_length - 1;
    scrobble_copy(last, s->queue[cnt]);
    _log (debug, "last.fm::popping_scrobble(%p//%u) %s//%s//%s", s->queue[cnt], cnt, last->title, last->artist, last->album);
    scrobble_free(s->queue[cnt]);

    s->queue_length--;
    s->previous = s->queue[cnt-1];

    return last;
}

scrobble* scrobbles_peek_queue(scrobbles *s, size_t i)
{
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

    scrobble_init(s);

    s->title = p->metadata.title;
    s->artist = p->metadata.artist;
    s->album = p->metadata.album;
    if (p->metadata.length > 0) {
        s->length = p->metadata.length / 1000000;
    } else {
        s->length = 60;
    }
    s->position = p->position;
    s->scrobbled = false;
    s->track_number = p->metadata.track_number;
    s->start_time = tstamp;
}

bool scrobble_is_valid(scrobble *s)
{
    if (NULL == s) { return false; }
    if (NULL == s->title) { return false; }
    if (NULL == s->album) { return false; }
    if (NULL == s->artist) { return false; }
    if (s->scrobbled) { return false; }
//    if (s->position == 0 && s->end_time > s->start_time) {
//        s->position = (s->end_time - s->start_time);
//    }
    return (
        strlen(s->title) > 0 &&
        strlen(s->artist) > 0 &&
        strlen(s->album) > 0 &&
        s->length >= LASTFM_MIN_TRACK_LENGTH
        //s->position >= s->length / 2
    );
}

bool now_playing_is_valid(const scrobble *m) {
    return (
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

void lastfm_destroy_scrobbler(lastfm_scrobbler *s)
{
    if (NULL == s) { return; }
    destroy_scrobbler(s);
}

void lastfm_scrobble(lastfm_scrobbler *s, const scrobble track)
{
    if (NULL == s) { return; }

    finished_playing(s);
    _log(info, "last.fm::scrobble %s//%s//%s", track.title, track.album, track.artist);
}

void lastfm_now_playing(lastfm_scrobbler *s, const scrobble track)
{
    if (NULL == s) { return; }

    submission_info *scrobble = create_submission_info();
    if (NULL == scrobble) { return; }

    scrobble->track = track.title;
    scrobble->artist = track.artist;
    scrobble->album = track.album;
    scrobble->track_length_in_secs = track.length;
    scrobble->time_started = track.start_time;

    started_playing (s, scrobble);

    _log(info, "last.fm::now_playing: %s//%s//%s", track.title, track.album, track.artist);
}

size_t consume_queue(scrobbles *s, lastfm_credentials *credentials)
{
    size_t consumed = 0;
    for (size_t i = 0; i < s->queue_length; i++) {
        scrobble *current = scrobbles_pop(s);

        consumed++;
        if (scrobble_is_valid(current)) {
            _log(info, "last.fm::scrobble:%s (%s//%s//%s)", credentials->user_name, current->title, current->album, current->artist);
            //lastfm_scrobble(credentials->user_name, credentials->password, current);
        } else {
            _log (warning, "base::pop_invalid_scrobble(%p//%u) %s:%s:%s", current, i, current->title, current->artist, current->album);
            continue;
        }
    }
    return consumed;
}

bool scrobbles_has_previous(const scrobbles *s)
{
    if(NULL != s) { return false; }
    return (NULL != s->previous);
}
