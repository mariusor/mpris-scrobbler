/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#include <clastfm.h>
#include <time.h>

#define API_KEY "990909c4e451d6c1ee3df4f5ee87e6f4"
#define API_SECRET "8bde8774564ef206edd9ef9722742a72"

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
    unsigned length;
    uint64_t position;
    time_t start_time;
    unsigned track_number;
} scrobble;


scrobble load_scrobble(mpris_properties *p, time_t start_time)
{
    scrobble s;
    if (NULL == p) { return s; }

    s.title = p->metadata.title;
    s.artist = p->metadata.artist;
    s.album = p->metadata.album;
    s.length = p->metadata.length / 1000000;
    s.position = p->position;
    s.track_number = p->metadata.track_number;
    s.start_time = start_time;

    return s;
}

bool scrobble_is_valid(scrobble *s) {
    if (s->position == 0) {
        time_t current = time(0);
        s->position = (current - s->start_time);
    }
    if (s->length < 30) {
        return false;
    }
    if (s->length == 0) {
        s->length = 240;
    }
    return (
        NULL != s->title && strlen(s->title) > 0 &&
        NULL != s->artist && strlen(s->artist) > 0 &&
        NULL != s->album && strlen(s->album) > 0 &&
        (s->position > (s->length / 2))
    );
}

int lastfm_scrobble(char* user_name, char* password, scrobble *track)
{
    int response_code;

    LASTFM_SESSION *s = LASTFM_init(API_KEY, API_SECRET);

    int rv = LASTFM_login(s, user_name, password);
    if (rv) {
        LASTFM_dinit(s);
        _log(error, "last.fm::login: failed");
        return rv;
    } else {
        _log(debug, "last.fm::login: %s", get_lastfm_status_label(rv));
    }

    /* Scrobble API 2.0 */
    response_code = LASTFM_track_scrobble(s, track->title, track->album, track->artist,
		track->start_time, track->length, track->track_number, 0, NULL);

    log_level log_as = info;
    const char* status = get_lastfm_status_label(response_code);

    if (response_code) { log_as = error; }
#if DEBUG
    _log(log_as, "last.fm::scrobble:%s:%d:%s (%s - %s - %s)", user_name, response_code, status, track->title, track->album, track->artist, password);
#else
    _log(log_as, "last.fm::scrobble:%s:%d:%s", user_name, response_code, status);
#endif

    LASTFM_dinit(s);
    return response_code;
}

typedef struct now_playing {
    char* title;
    char* album;
    char* artist;
    unsigned length;
    unsigned track_number;
} now_playing;

now_playing load_now_playing(mpris_properties *p)
{
    now_playing m;
    if (NULL == p) { return m; }

    m.title = p->metadata.title;
    m.artist = p->metadata.artist;
    m.album = p->metadata.album;

    return m;
}

bool now_playing_is_valid(now_playing *m) {
    return (
        NULL != m->title && strlen(m->title) > 0 &&
        NULL != m->artist && strlen(m->artist) > 0 &&
        NULL != m->album && strlen(m->album) > 0
    );
}

int lastfm_now_playing(char* user_name, char* password, now_playing *track)
{
    int response_code;

    LASTFM_SESSION *s = LASTFM_init(API_KEY, API_SECRET);

    int rv = LASTFM_login(s, user_name, password);
    if (rv) {
        LASTFM_dinit(s);
        _log(error, "last.fm::login: failed");
        return rv;
    } else {
        _log(debug, "last.fm::login: %s", get_lastfm_status_label(rv));
    }

    /* Scrobble API 2.0 */
    response_code = LASTFM_track_update_now_playing(s, track->title, track->album, track->artist, track->length, track->track_number, 0, NULL);
    /* Get a pointer to the internal status message buffer */
    log_level log_as = info;
    const char* status = get_lastfm_status_label(response_code);

    if (response_code) { log_as = error; }
#if DEBUG
    _log(log_as, "last.fm::now_playing:%s:%s (%s - %s - %s)", user_name, status, track->title, track->album, track->artist);
#else
    _log(log_as, "last.fm::now_playing:%s:%s", user_name, status);
#endif

    LASTFM_dinit(s);
    return response_code;
}
