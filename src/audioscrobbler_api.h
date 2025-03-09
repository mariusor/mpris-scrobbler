/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_AUDIOSCROBBLER_API_H
#define MPRIS_SCROBBLER_AUDIOSCROBBLER_API_H

#ifndef LASTFM_API_KEY
#include "credentials_lastfm.h"
#endif
#ifndef LIBREFM_API_KEY
#include "credentials_librefm.h"
#endif

#define MIN_SCROBBLE_DELAY_SECONDS        4.0L*60.0L

#define LASTFM_AUTH_URL            "www.last.fm"
#define LASTFM_AUTH_PATH           "api/auth/"
#define LASTFM_API_BASE_URL        "ws.audioscrobbler.com"
#define LASTFM_API_VERSION         "2.0"

#define LIBREFM_AUTH_URL            "libre.fm"
#define LIBREFM_AUTH_PATH           "api/auth/"
#define LIBREFM_API_BASE_URL        "libre.fm"
#define LIBREFM_API_VERSION         "2.0"

#define API_TOKEN_NODE_NAME             "token"
#define API_SESSION_NODE_NAME           "session"
#define API_NAME_NODE_NAME              "name"
#define API_KEY_NODE_NAME               "key"
#define API_SUBSCRIBER_NODE_NAME        "subscriber"
#define API_NOWPLAYING_NODE_NAME        "nowplaying"
#define API_SCROBBLES_NODE_NAME         "scrobbles"
#define API_SCROBBLE_NODE_NAME          "scrobble"
#define API_TIMESTAMP_NODE_NAME         "timestamp"
#define API_TRACK_NODE_NAME             "track"
#define API_ARTIST_NODE_NAME            "artist"
#define API_ALBUM_NODE_NAME             "album"
#define API_ALBUMARTIST_NODE_NAME       "albumArtist"
#define API_IGNORED_NODE_NAME           "ignoredMessage"
#define API_MUSICBRAINZ_MBID_NODE_NAME  "mbid"
#define API_STATUS_ATTR_NAME            "status"
#define API_STATUS_VALUE_OK             "ok"
#define API_STATUS_VALUE_FAILED         "failed"
#define API_ERROR_NODE_NAME             "error"
#define API_ERROR_MESSAGE_NAME          "message"
#define API_ERROR_CODE_ATTR_NAME        "code"

#define API_METHOD_NOW_PLAYING          "track.updateNowPlaying"
#define API_METHOD_SCROBBLE             "track.scrobble"
#define API_METHOD_GET_TOKEN            "auth.getToken"
#define API_METHOD_GET_SESSION          "auth.getSession"

enum api_node_types {
    api_node_type_unknown,
    // all
    api_node_type_document,
    api_node_type_root,
    api_node_type_error,
    // auth.getToken
    api_node_type_token,
    // track.nowPlaying
    api_node_type_now_playing,
    api_node_type_track,
    api_node_type_artist,
    api_node_type_album,
    api_node_type_album_artist,
    api_node_type_ignored_message,
    // track.scrobble
    api_node_type_scrobbles,
    api_node_type_scrobble,
    api_node_type_timestamp,
    // auth.getSession
    api_node_type_session,
    api_node_type_session_name,
    api_node_type_session_key,
    api_node_type_session_subscriber,
} api_node_type;

enum api_return_code {
    unavaliable             = 1, //The service is temporarily unavailable, please try again.
    invalid_service         = 2, //Invalid service - This service does not exist
    invalid_method          = 3, //Invalid Method - No method with that name in this package
    authentication_failed   = 4, //Authentication Failed - You do not have permissions to access the service
    invalid_format          = 5, //Invalid format - This service doesn't exist in that format
    invalid_parameters      = 6, //Invalid parameters - Your request is missing a required parameter
    invalid_resource        = 7, //Invalid resource specified
    operation_failed        = 8, //Operation failed - Something else went wrong
    invalid_session_key     = 9, //Invalid session key - Please re-authenticate
    invalid_apy_key         = 10, //Invalid API key - You must be granted a valid key by last.fm
    service_offline         = 11, //Service Offline - This service is temporarily offline. Try again later.
    invalid_signature       = 13, //Invalid method signature supplied
    temporary_error         = 16, //There was a temporary error processing your request. Please try again
    suspended_api_key       = 26, //Suspended API key - Access for your account has been suspended, please contact Last.fm
    rate_limit_exceeded     = 29, //Rate limit exceeded - Your IP has made too many requests in a short period
};

struct api_error {
    char *message;
    enum api_return_code code;
};

static bool audioscrobbler_now_playing_is_valid(const struct scrobble *m/*, const time_t last_playing_time*/) {
    if (NULL == m) {
        return false;
    }

    if (_is_zero(m->artist)) { return false; }
    const bool result = (
            strlen(m->title) > 0LU &&
            strlen(m->artist[0]) > 0LU &&
            strlen(m->album) > 0LU &&
            // last_playing_time > 0LU &&
            // difftime(current_time, last_playing_time) >= LASTFM_NOW_PLAYING_DELAY &&
            m->length > 0.0L &&
            m->position <= (double)m->length
    );

    return result;
}

static double min_scrobble_delay_seconds(const struct scrobble *);
static bool audioscrobbler_scrobble_is_valid(const struct scrobble *s)
{
    if (NULL == s) { return false; }
    if (_is_zero(s->artist)) { return false; }

    const double scrobble_interval = min_scrobble_delay_seconds(s);
    double d;
    if (s->play_time > 0) {
        d = s->play_time +1lu;
    } else {
        const time_t now = time(NULL);
        d = difftime(now, s->start_time) + 1lu;
    }

    const bool result = (
        s->length >= (double)MIN_TRACK_LENGTH &&
        d >= scrobble_interval &&
        strlen(s->title) > 0 &&
        strlen(s->artist[0]) > 0 &&
        strlen(s->album) > 0
    );
    return result;
}

static void audioscrobbler_api_response_get_session_key_json(const char *buffer, const size_t length, struct api_credentials *credentials)
{
    if (NULL == buffer) { return; }
    if (length == 0) { return; }

    struct json_tokener *tokener = json_tokener_new();
    if (NULL == tokener) { return; }
    json_object *root = json_tokener_parse_ex(tokener, buffer, (int)length);
    if (NULL == root) {
        _warn("json::invalid_json_message");
        goto _exit;
    }
    if (json_object_object_length(root) < 1) {
        _warn("json::no_root_object");
        goto _exit;
    }
    json_object *sess_object = NULL;
    if (!json_object_object_get_ex(root, API_SESSION_NODE_NAME, &sess_object) || NULL == sess_object) {
        _warn("json:missing_session_object");
        goto _exit;
    }
    if (!json_object_is_type(sess_object, json_type_object)) {
        _warn("json::session_is_not_object");
        goto _exit;
    }
    json_object *key_object = NULL;
    if (!json_object_object_get_ex(sess_object, API_KEY_NODE_NAME, &key_object) || NULL == key_object) {
        _warn("json:missing_key");
        goto _exit;
    }
    if (!json_object_is_type(key_object, json_type_string)) {
        _warn("json::key_is_not_string");
        goto _exit;
    }
    const char *session_key = json_object_get_string(key_object);
    memcpy((char*)credentials->session_key, session_key, strlen(session_key));
    _info("json::loaded_session_key: %s", session_key);

    json_object *name_object = NULL;
    if (!json_object_object_get_ex(sess_object, API_NAME_NODE_NAME, &name_object) || NULL == name_object) {
        goto _exit;
    }
    if (!json_object_is_type(name_object, json_type_string)) {
        goto _exit;
    }
    const char *name = json_object_get_string(name_object);
    memcpy((char*)credentials->user_name, name, min(USER_NAME_MAX, strlen(name)));
    _info("json::loaded_session_user: %s", name);

_exit:
    if (NULL != root) { json_object_put(root); }
    json_tokener_free(tokener);
}

static void audioscrobbler_api_response_get_token_json(const char *buffer, const size_t length, struct api_credentials *credentials)
{
    if (NULL == buffer) { return; }
    if (length == 0) { return; }

    struct json_tokener *tokener = json_tokener_new();
    json_object *root = json_tokener_parse_ex(tokener, buffer, (int)length);
    if (NULL == root) {
        _warn("json::invalid_json_message");
        goto _exit;
    }
    if (json_object_object_length(root) < 1) {
        _warn("json::no_root_object");
        goto _exit;
    }
    json_object *tok_object = NULL;
    if (!json_object_object_get_ex(root, API_TOKEN_NODE_NAME, &tok_object) || NULL == tok_object) {
        _warn("json:missing_token_key");
        goto _exit;
    }
    if (!json_object_is_type(tok_object, json_type_string)) {
        _warn("json::token_is_not_string");
        goto _exit;
    }
    const char *value = json_object_get_string(tok_object);
    memcpy((char*)credentials->token, value, min(MAX_SECRET_LENGTH, strlen(value)));
    _info("json::loaded_token: %s", value);

_exit:
    if (NULL != root) { json_object_put(root); }
    json_tokener_free(tokener);
}

static bool audioscrobbler_json_document_is_error(const char *buffer, const size_t length)
{
    // {"error":14,"message":"This token has not yet been authorised"}
    bool result = false;

    if (NULL == buffer) { return result; }
    if (length == 0) { return result; }

    json_object *err_object = NULL;
    json_object *msg_object = NULL;

    struct json_tokener *tokener = json_tokener_new();
    if (NULL == tokener) { return result; }
    json_object *root = json_tokener_parse_ex(tokener, buffer, (int)length);

    if (NULL == root || json_object_object_length(root) < 1) {
        goto _exit;
    }
    json_object_object_get_ex(root, API_ERROR_NODE_NAME, &err_object);
    json_object_object_get_ex(root, API_ERROR_MESSAGE_NAME, &msg_object);
    if (NULL == err_object || !json_object_is_type(err_object, json_type_int)) {
        goto _exit;
    }
    if (NULL == msg_object || !json_object_is_type(msg_object, json_type_string)) {
        goto _exit;
    }
    result = true;

_exit:
    if (NULL != root) { json_object_put(root); }
    json_tokener_free(tokener);
    return result;
}

static bool audioscrobbler_valid_api_credentials(const struct api_credentials *auth)
{
    if (NULL == auth) { return false; }
    if (auth->end_point != api_lastfm && auth->end_point != api_librefm) { return false; }
    if (strlen(auth->api_key) == 0) { return false; }
    if (strlen(auth->secret) == 0) { return false; }
    return true;
}

static bool audioscrobbler_valid_credentials(const struct api_credentials *auth)
{
    if (NULL == auth) { return false; }
    return audioscrobbler_valid_api_credentials(auth) && auth->enabled;
}

#define MD5_DIGEST_LENGTH 16
#define MD5_HEX_LENGTH (2 * MD5_DIGEST_LENGTH + 2)
static void api_get_signature(const char *string, const char *secret, char *result)
{
    if (NULL == string) { return; }
    if (NULL == secret) { return; }
    if (NULL == result) { return; }
    const size_t string_len = strlen(string);
    const size_t secret_len = strlen(secret);
    const size_t len = string_len + secret_len;
    char sig[MAX_BODY_SIZE+1] = {0};

    // TODO(marius): this needs to change to memcpy or strncpy
    strncat(sig, string, MAX_BODY_SIZE);
    strncat(sig, secret, MAX_PROPERTY_LENGTH/2);

    unsigned char sig_hash[MD5_DIGEST_LENGTH] = {0};

    md5((uint8_t*)sig, len, sig_hash);

    for (size_t n = 0; n < MD5_DIGEST_LENGTH; n++) {
        snprintf(result + 2 * n, 3, "%02x", sig_hash[n]);
    }
}

static void append_method_query_param(CURL *url, const char *method, char *sig_base)
{
    char query_param[MAX_URL_LENGTH + 1] = {0};
    snprintf(query_param, MAX_URL_LENGTH, "method=%s", method);
    curl_url_set(url, CURLUPART_QUERY, query_param, CURLU_APPENDQUERY);

    strncat(sig_base, "method", 7);
    // NOTE(marius): 22 is the length of the longest method: API_METHOD_NOW_PLAYING
    strncat(sig_base, method, 22);
}

static void append_api_key_query_param(CURL *url, const char *api_key, CURL *handle, char *sig_base)
{
    char query_param[MAX_URL_LENGTH + 1] = {0};
    snprintf(query_param, MAX_URL_LENGTH, "api_key=%s", api_key);
    curl_url_set(url, CURLUPART_QUERY, query_param, CURLU_APPENDQUERY|CURLU_URLENCODE);

    if (NULL != sig_base && NULL != handle) {
        char *escaped_api_key = curl_easy_escape(handle, api_key, (int)strlen(api_key));
        const size_t escaped_key_len = strlen(escaped_api_key);

        strncat(sig_base, "api_key", 8);
        strncat(sig_base, escaped_api_key, escaped_key_len + 1);
        curl_free(escaped_api_key);
    }
}

static void append_signature_query_param(CURL *url, const char *sig_base, const char *secret)
{
    char sig[MD5_HEX_LENGTH] = {0};
    api_get_signature(sig_base, secret, sig);

    char query_param[MAX_URL_LENGTH + 1] = {0};
    snprintf(query_param, MAX_URL_LENGTH, "api_sig=%s", sig);
    curl_url_set(url, CURLUPART_QUERY, query_param, CURLU_APPENDQUERY);
}

static void api_get_url(CURLU*, const struct api_endpoint*);
struct api_endpoint *api_endpoint_new(const struct api_credentials*);
struct http_request *http_request_new(void);
/*
 * api_key (Required) : A Last.fm API key.
 * api_sig (Required) : A Last.fm method signature. See [authentication](https://www.last.fm/api/authentication) for more information.
*/
static struct http_request *audioscrobbler_api_build_request_get_token(const struct api_credentials *auth, CURL *handle)
{
    if (!audioscrobbler_valid_api_credentials(auth)) { return NULL; }

    struct http_request *request = http_request_new();

    char sig_base[MAX_BODY_SIZE+1] = {0};

    append_api_key_query_param(request->url, auth->api_key, handle, sig_base);
    append_method_query_param(request->url, API_METHOD_GET_TOKEN, sig_base);
    append_signature_query_param(request->url, sig_base, auth->secret);

    curl_url_set(request->url, CURLUPART_QUERY, "format=json", CURLU_APPENDQUERY);

    request->request_type = http_post;
    request->end_point = api_endpoint_new(auth);
    api_get_url(request->url, request->end_point);

    return request;
}

static void append_token_query_param(CURL *url, const char *token, char *sig_base)
{
    const size_t token_len = strlen(token);
    char query_param[MAX_URL_LENGTH + 1] = {0};
    snprintf(query_param, MAX_URL_LENGTH, "token=%s", token);
    curl_url_set(url, CURLUPART_QUERY, query_param, CURLU_APPENDQUERY);

    if (NULL != sig_base) {
        strncat(sig_base, "token", 6);
        strncat(sig_base, token, token_len + 1);
    }
}

/*
 * token (Required) : A 32-character ASCII hexadecimal MD5 hash returned by step 1 of the authentication process (following the granting of permissions to the application by the user)
 * api_key (Required) : A Last.fm API key.
 * api_sig (Required) : A Last.fm method signature. See authentication for more information.
 * Return codes
 *  2 : Invalid service - This service does not exist
 *  3 : Invalid Method - No method with that name in this package
 *  4 : Authentication Failed - You do not have permissions to access the service
 *  4 : Invalid authentication token supplied
 *  5 : Invalid format - This service doesn't exist in that format
 *  6 : Invalid parameters - Your request is missing a required parameter
 *  7 : Invalid resource specified
 *  8 : Operation failed - Something else went wrong
 *  9 : Invalid session key - Please re-authenticate
 * 10 : Invalid API key - You must be granted a valid key by last.fm
 * 11 : Service Offline - This service is temporarily offline. Try again later.
 * 13 : Invalid method signature supplied
 * 14 : This token has not been authorized
 * 15 : This token has expired
 * 16 : There was a temporary error processing your request. Please try again
 * 26 : Suspended API key - Access for your account has been suspended, please contact Last.fm
 * 29 : Rate limit exceeded - Your IP has made too many requests in a short period*
 */
static struct http_request *audioscrobbler_api_build_request_get_session(const struct api_credentials *auth, CURL *handle)
{
    if (!audioscrobbler_valid_api_credentials(auth)) { return NULL; }

    struct http_request *request = http_request_new();

    request->request_type = http_post;
    request->end_point = api_endpoint_new(auth);
    api_get_url(request->url, request->end_point);

    char sig_base[MAX_BODY_SIZE] = {0};

    append_api_key_query_param(request->url, auth->api_key, handle, sig_base);
    append_method_query_param(request->url, API_METHOD_GET_SESSION, sig_base);
    append_token_query_param(request->url, auth->token, sig_base);
    append_signature_query_param(request->url, sig_base, auth->secret);
    curl_url_set(request->url, CURLUPART_QUERY, "format=json", CURLU_APPENDQUERY);

    return request;

    return NULL;
}

/*
 * artist (Required) : The artist name.
 * track (Required) : The track name.
 * album (Optional) : The album name.
 * trackNumber (Optional) : The track number of the track on the album.
 * context (Optional) : Sub-client version (not public, only enabled for certain API keys)
 * mbid (Optional) : The MusicBrainz Track ID.
 * duration (Optional) : The length of the track in seconds.
 * albumArtist (Optional) : The album artist - if this differs from the track artist.
 * api_key (Required) : A Last.fm API key.
 * api_sig (Required) : A Last.fm method signature. See authentication for more information.
 * sk (Required) : A session key generated by authenticating a user via the authentication protocol.
 */
static struct http_request *audioscrobbler_api_build_request_now_playing(const struct scrobble *tracks[], const unsigned track_count, const struct api_credentials *auth, CURL *handle)
{
    if (!audioscrobbler_valid_credentials(auth)) { return NULL; }

    (void)track_count; // quiet -Wunused-parameter
    assert(track_count == 1);

    const struct scrobble *track = tracks[0];
    const char *api_key = auth->api_key;
    const char *secret = auth->secret;
    const char *sk = auth->session_key;

    struct http_request *request = http_request_new();

    char sig_base[MAX_BODY_SIZE+1] = {0};
    char *body = get_zero_string(MAX_BODY_SIZE);
    if (NULL == body) { goto _failure; }

    assert(track->album);
    const size_t album_len = strlen(track->album);
    char *esc_album = curl_easy_escape(handle, track->album, (int)album_len);

    strncat(body, "album=", 7);
    strncat(body, esc_album, grrrs_cap(body));
    strncat(body, "&", 2);

    strncat(sig_base, "album", 6);
    strncat(sig_base, track->album, MAX_BODY_SIZE);
    curl_free(esc_album);

    assert(api_key);
    const size_t api_key_len = strlen(api_key);
    char *esc_api_key = curl_easy_escape(handle, api_key, (int)api_key_len);

    strncat(body, "api_key=", 9);
    strncat(body, esc_api_key, grrrs_cap(body));
    strncat(body, "&", 2);

    strncat(sig_base, "api_key", 8);
    strncat(sig_base, api_key, MAX_BODY_SIZE);
    curl_free(esc_api_key);

    char full_artist[MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT + 1] = {0};
    size_t full_artist_len = 0;
    for (size_t i = 0; i < array_count(track->artist); i++) {
        const char *artist = track->artist[i];
        const size_t artist_len = strlen(artist);
        if (NULL == artist || artist_len == 0) { continue; }

        if (full_artist_len > 0) {
            size_t l_val_sep = strlen(VALUE_SEPARATOR);
            strncat(full_artist, VALUE_SEPARATOR, l_val_sep + 1);
            full_artist_len += l_val_sep;
        }
        strncat(full_artist, artist, artist_len + 1);
        full_artist_len += artist_len;
    }
    if (full_artist_len > 0) {
        char *esc_full_artist = curl_easy_escape(handle, full_artist, (int)full_artist_len);
        const size_t artist_label_len = strlen(API_ARTIST_NODE_NAME);

        strncat(body, API_ARTIST_NODE_NAME "=", artist_label_len + 2);
        strncat(body, esc_full_artist, grrrs_cap(body));
        strncat(body, "&", 2);

        strncat(sig_base, API_ARTIST_NODE_NAME, artist_label_len + 1);
        strncat(sig_base, full_artist, MAX_BODY_SIZE);
        curl_free(esc_full_artist);
    }

    char *mb_track_id = (char *) track->mb_track_id[0];
    const size_t mbid_len = strlen(mb_track_id);
    if (mbid_len > 0) {
        char *esc_mbid = curl_easy_escape(handle, mb_track_id, (int)mbid_len);

        const size_t mbid_label_len = strlen(API_MUSICBRAINZ_MBID_NODE_NAME);

        strncat(body, API_MUSICBRAINZ_MBID_NODE_NAME "=", mbid_label_len + 2);
        strncat(body, esc_mbid, MAX_BODY_SIZE);
        strncat(body, "&", 2);

        strncat(sig_base, API_MUSICBRAINZ_MBID_NODE_NAME, mbid_label_len + 1);
        strncat(sig_base, mb_track_id, MAX_BODY_SIZE);
        curl_free(esc_mbid);
    }

    const char *method = API_METHOD_NOW_PLAYING;

    assert(method);
    const size_t method_len = strlen(method);
    strncat(body, "method=", 8);
    strncat(body, method, method_len + 1);
    strncat(body, "&", 2);

    strncat(sig_base, "method", 7);
    strncat(sig_base, method, method_len + 1);

    strncat(body, "sk=", 4);
    strncat(body, sk, MAX_PROPERTY_LENGTH);
    strncat(body, "&", 2);

    strncat(sig_base, "sk", 3);
    strncat(sig_base, sk, MAX_SECRET_LENGTH);

    assert(track->title);
    const size_t title_len = strlen(track->title);
    char *esc_title = curl_easy_escape(handle, track->title, (int)title_len);

    strncat(body, "track=", 7);
    strncat(body, esc_title, MAX_BODY_SIZE);
    strncat(body, "&", 2);

    strncat(sig_base, "track", 6);
    strncat(sig_base, track->title, title_len + 1);
    curl_free(esc_title);

    char sig[MD5_HEX_LENGTH] = {0};
    api_get_signature(sig_base, secret, sig);
    strncat(body, "api_sig=", 9);
    strncat(body, sig, MAX_PROPERTY_LENGTH);

    curl_url_set(request->url, CURLUPART_QUERY, "format=json", CURLU_APPENDQUERY);

    request->request_type = http_post;
    request->body = body;
    request->body_length = strlen(body);
    request->end_point = api_endpoint_new(auth);
    api_get_url(request->url, request->end_point);
    return request;

_failure:
    if (NULL != body) { grrrs_free(body); }
    return NULL;
}

static bool scrobble_is_empty(const struct scrobble*);
static struct http_request *audioscrobbler_api_build_request_scrobble(const struct scrobble *tracks[MAX_QUEUE_LENGTH], const unsigned track_count, const struct api_credentials *auth, CURL *handle)
{
    if (!audioscrobbler_valid_credentials(auth)) { return NULL; }

    const char *api_key = auth->api_key;
    const char *secret = auth->secret;
    const char *sk = auth->session_key;

    struct http_request *request = http_request_new();

    const char *method = API_METHOD_SCROBBLE;

    char sig_base[MAX_BODY_SIZE+1] = {0};
    char *body = get_zero_string(MAX_BODY_SIZE);
    if (NULL == body) { return NULL; }

    for (size_t i = 0; i < track_count; i++) {
        const struct scrobble *track = tracks[i];

        if (scrobble_is_empty(track)) {
            continue;
        }
        const size_t album_len = strlen(track->album);

        char *esc_album = curl_easy_escape(handle, track->album, (int)album_len);
        char album_body[MAX_PROPERTY_LENGTH] = {0};
        snprintf(album_body, MAX_PROPERTY_LENGTH, API_ALBUM_NODE_NAME "[%lu]=%s&", i, esc_album);
        strncat(body, album_body, MAX_PROPERTY_LENGTH);

        char album_sig[MAX_PROPERTY_LENGTH + 19] = {0};
        snprintf(album_sig, MAX_PROPERTY_LENGTH + 18, API_ALBUM_NODE_NAME "[%lu]%s", i, track->album);

        assert(strlen(sig_base) + strlen(album_sig)<MAX_BODY_SIZE);
        strncat(sig_base, album_sig, MAX_PROPERTY_LENGTH + 19);

        curl_free(esc_album);
    }

    assert(api_key);
    const size_t api_key_len = strlen(api_key);
    char *esc_api_key = curl_easy_escape(handle, api_key, (int)api_key_len);

    strncat(body, "api_key=", 9);
    strncat(body, esc_api_key, grrrs_cap(body));
    strncat(body, "&", 2);

    strncat(sig_base, "api_key", 8);
    strncat(sig_base, api_key, MAX_BODY_SIZE);
    curl_free(esc_api_key);

    for (size_t i = 0; i < track_count; i++) {
        const struct scrobble *track = tracks[i];

        char full_artist[MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT] = {0};
        size_t full_artist_len = 0;
        for (unsigned j = 0; j < array_count(track->artist); j++) {
            const char *artist = track->artist[j];
            const size_t artist_len = strlen(artist);
            if (NULL == artist || artist_len == 0) { continue; }

            if (full_artist_len > 0) {
                const size_t l_val_sep = strlen(VALUE_SEPARATOR);
                strncat(full_artist, VALUE_SEPARATOR, l_val_sep + 1);
                full_artist_len += l_val_sep;
            }
            strncat(full_artist, artist, artist_len + 1);
            full_artist_len += artist_len;
        }
        if (full_artist_len > 0) {
            char *esc_full_artist = curl_easy_escape(handle, full_artist, (int)full_artist_len);

            const char fmt_full_artist[] = API_ARTIST_NODE_NAME "[%zu]=%s&";

            char artist_body[MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT + 11] = {0};
            snprintf(artist_body, MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT + 11, fmt_full_artist, i, esc_full_artist);

            strncat(body, artist_body, MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT + 11);

            const char fmt_artist_sig[] = API_ARTIST_NODE_NAME "[%zu]%s";
            char artist_sig[MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT + 9] = {0};
            snprintf(artist_sig, MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT + 9, fmt_artist_sig, i, full_artist);

            assert(strlen(sig_base) + strlen(artist_sig) < MAX_BODY_SIZE);
            strncat(sig_base, artist_sig, MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT + 9);
            curl_free(esc_full_artist);
        }
    }

    for (size_t i = 0; i < track_count; i++) {
        const struct scrobble *track = tracks[i];

        char *mb_track_id = (char*)track->mb_track_id[0];
        const size_t mbid_len = strlen(mb_track_id);
        if (mbid_len > 0) {
            char *esc_mbid = curl_easy_escape(handle, mb_track_id, (int)mbid_len);

            char mbid_body[MAX_PROPERTY_LENGTH] = {0};
            snprintf(mbid_body, MAX_PROPERTY_LENGTH, API_MUSICBRAINZ_MBID_NODE_NAME "[%lu]=%s&", i, esc_mbid);
            strncat(body, mbid_body, MAX_PROPERTY_LENGTH);

            char mbid_sig[MAX_PROPERTY_LENGTH + 18] = {0};
            snprintf(mbid_sig, MAX_PROPERTY_LENGTH + 17, API_MUSICBRAINZ_MBID_NODE_NAME "[%lu]%s", i, mb_track_id);

            assert(strlen(sig_base) + strlen(mbid_sig) < MAX_BODY_SIZE);
            strncat(sig_base, mbid_sig, MAX_PROPERTY_LENGTH + 18);

            curl_free(esc_mbid);
        }
    }

    assert(method);
    const size_t method_len = strlen(method);
    strncat(body, "method=", 8);
    strncat(body, method, method_len + 1);
    strncat(body, "&", 2);

    strncat(sig_base, "method", 7);
    assert(strlen(sig_base) + strlen(method) < MAX_BODY_SIZE);
    strncat(sig_base, method, method_len + 1);

    assert(sk);
    strncat(body, "sk=", 4);
    strncat(body, sk, MAX_SECRET_LENGTH+1);
    strncat(body, "&", 2);

    strncat(sig_base, "sk", 3);
    assert(strlen(sig_base) + strlen(sk) < MAX_BODY_SIZE);
    strncat(sig_base, sk, MAX_SECRET_LENGTH+1);

    for (int i = (int)track_count - 1; i >= 0; i--) {
        const struct scrobble *track = tracks[i];

        char tstamp_body[MAX_PROPERTY_LENGTH] = {0};
        snprintf(tstamp_body, MAX_PROPERTY_LENGTH, API_TIMESTAMP_NODE_NAME "[%d]=%ld&", i, track->start_time);
        strncat(body, tstamp_body, MAX_PROPERTY_LENGTH);

        char tstamp_sig[MAX_PROPERTY_LENGTH] = {0};
        snprintf(tstamp_sig, MAX_PROPERTY_LENGTH, API_TIMESTAMP_NODE_NAME "[%d]%ld", i, track->start_time);

        assert(strlen(sig_base) + strlen(tstamp_sig) < MAX_BODY_SIZE);
        strncat(sig_base, tstamp_sig, MAX_PROPERTY_LENGTH);
    }

    for (int i = (int)track_count - 1; i >= 0; i--) {
        const struct scrobble *track = tracks[i];

        const size_t title_len = strlen(track->title);

        char *esc_title = curl_easy_escape(handle, track->title, (int)title_len);

        char title_body[MAX_PROPERTY_LENGTH] = {0};
        snprintf(title_body, MAX_PROPERTY_LENGTH, API_TRACK_NODE_NAME "[%d]=%s&", i, esc_title);
        strncat(body, title_body, MAX_PROPERTY_LENGTH);

        char title_sig[MAX_PROPERTY_LENGTH + 19] = {0};
        snprintf(title_sig, MAX_PROPERTY_LENGTH + 18, API_TRACK_NODE_NAME "[%d]%s", i, track->title);

        assert(strlen(sig_base) + strlen(title_sig) < MAX_BODY_SIZE);
        strncat(sig_base, title_sig, MAX_PROPERTY_LENGTH + 19);

        curl_free(esc_title);
    }

    char sig[MD5_HEX_LENGTH] = {0};
    api_get_signature(sig_base, secret, sig);
    strncat(body, "api_sig=", 9);
    strncat(body, sig, MAX_PROPERTY_LENGTH);

    curl_url_set(request->url, CURLUPART_QUERY, "format=json", CURLU_APPENDQUERY);

    request->request_type = http_post;
    request->body = body;
    request->body_length = strlen(body);
    request->end_point = api_endpoint_new(auth);
    api_get_url(request->url, request->end_point);

    return request;
}

#endif // MPRIS_SCROBBLER_AUDIOSCROBBLER_API_H
