/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#include <clastfm.h>

#define API_KEY "990909c4e451d6c1ee3df4f5ee87e6f4"
#define API_SECRET "8bde8774564ef206edd9ef9722742a72"

int _log(log_level level, const char* format, ...);

typedef enum lastfm_call_statuses {
    ok = 0,
    unavaliable = 16, //The service is temporarily unavailable, please try again.
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

//lastfm_call_statuses get_lastfm_status_label (

typedef struct now_playing {
    char* title;
    char* album;
    char* artist;
    unsigned length;
    unsigned track_number;
} now_playing;

int lastfm_scrobble()
{
    return 0;
}

int lastfm_now_playing(char* user_name, char* password, now_playing *track)
{
    const char *status = NULL;
    int response_code;

    LASTFM_SESSION *s = LASTFM_init(API_KEY, API_SECRET);

    /* Get a pointer to the internal status message buffer */
    LASTFM_status(s, &status, NULL, NULL);

    int rv = LASTFM_login(s, user_name, password);
    if (rv) {
        free (s);
        _log(error, "last.fm::login: failed");
        return rv;
    } else {
        _log(debug, "last.fm::login: %s", status);
    }

    /* Scrobble API 2.0 */
    response_code = LASTFM_track_update_now_playing(s, track->title, track->album, track->artist, track->length, track->track_number, 0, NULL);
    /* Get a pointer to the internal status message buffer */
    LASTFM_status(s, &status, NULL, NULL);
    log_level log_as = info;
    if (response_code) {
        log_as = error;
    }
#if DEBUG
    _log(log_as, "last.fm::now_playing:%s:%s (%s - %s - %s)", user_name, status, track->title, track->album, track->artist);
#else
    _log(log_as, "last.fm::now_playing:%s:%s", user_name, status);
#endif

    free(s);
    return response_code;
}
