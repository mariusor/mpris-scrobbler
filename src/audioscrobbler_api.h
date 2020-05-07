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

#define MIN_SCROBBLE_MINUTES        4

#define LASTFM_AUTH_URL            "www.last.fm"
#define LASTFM_AUTH_PATH           "api/auth/?api_key=%s&token=%s"
#define LASTFM_API_BASE_URL        "ws.audioscrobbler.com"
#define LASTFM_API_VERSION         "2.0"

#define LIBREFM_AUTH_URL            "libre.fm"
#define LIBREFM_AUTH_PATH           "api/auth/?api_key=%s&token=%s"
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
    enum api_return_code code;
    char *message;
};

void audioscrobbler_api_response_get_session_key_json(const char *buffer, const size_t length, char **session_key, char **name)
{
    if (NULL == buffer) { return; }
    if (length == 0) { return; }
    if (NULL == *session_key) { return; }
    if (NULL == *name) { return; }

    struct json_tokener *tokener = json_tokener_new();
    if (NULL == tokener) { return; }
    json_object *root = json_tokener_parse_ex(tokener, buffer, length);
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
    const char *sess_value = json_object_get_string(key_object);
    strncpy(*session_key, sess_value, MAX_PROPERTY_LENGTH);
    _info("json::loaded_session_key: %s", *session_key);

    json_object *name_object = NULL;
    if (!json_object_object_get_ex(sess_object, API_NAME_NODE_NAME, &name_object) || NULL == name_object) {
        goto _exit;
    }
    if (!json_object_is_type(name_object, json_type_string)) {
        goto _exit;
    }
    const char *name_value = json_object_get_string(name_object);
    strncpy(*name, name_value, MAX_PROPERTY_LENGTH);
    _info("json::loaded_session_user: %s", *name);

_exit:
    if (NULL != root) { json_object_put(root); }
    json_tokener_free(tokener);
}

void audioscrobbler_api_response_get_token_json(const char *buffer, const size_t length, char **token)
{
    // {"token":"NQH5C24A6RbIOx1xWUcty1N6yOHcKcRk"}
    if (NULL == buffer) { return; }
    if (length == 0) { return; }
    if (NULL == *token) { return; }

    struct json_tokener *tokener = json_tokener_new();
    json_object *root = json_tokener_parse_ex(tokener, buffer, length);
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
    strncpy(*token, value, MAX_PROPERTY_LENGTH);
    _info("json::loaded_token: %s", *token);

_exit:
    if (NULL != root) { json_object_put(root); }
    json_tokener_free(tokener);
}

bool audioscrobbler_json_document_is_error(const char *buffer, const size_t length)
{
    // {"error":14,"message":"This token has not yet been authorised"}
    bool result = false;

    if (NULL == buffer) { return result; }
    if (length == 0) { return result; }

    json_object *err_object = NULL;
    json_object *msg_object = NULL;

    struct json_tokener *tokener = json_tokener_new();
    if (NULL == tokener) { return result; }
    json_object *root = json_tokener_parse_ex(tokener, buffer, length);

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

static bool audioscrobbler_valid_credentials(const struct api_credentials *auth)
{
    bool status = false;
    if (NULL == auth) { return status; }
    if (auth->end_point != api_lastfm && auth->end_point != api_librefm) { return status; }

    status = true;
    return status;
}

char *api_get_url(struct api_endpoint*);
struct api_endpoint *api_endpoint_new(const struct api_credentials*);
char *api_get_signature(const char*, const char*);
struct http_request *http_request_new(void);

/*
 * api_key (Required) : A Last.fm API key.
 * api_sig (Required) : A Last.fm method signature. See [authentication](https://www.last.fm/api/authentication) for more information.
*/
struct http_request *audioscrobbler_api_build_request_get_token(const struct api_credentials *auth)
{
    if (!audioscrobbler_valid_credentials(auth)) { return NULL; }
    CURL *handle = curl_easy_init();

    const char *api_key = auth->api_key;
    const char *secret = auth->secret;

    struct http_request *request = http_request_new();

    char *sig_base = get_zero_string(MAX_BODY_SIZE);
    char *query = get_zero_string(MAX_BODY_SIZE);

    char *escaped_api_key = curl_easy_escape(handle, api_key, strlen(api_key));
    size_t escaped_key_len = strlen(escaped_api_key);
    strncat(query, "api_key=", 9);
    strncat(query, escaped_api_key, escaped_key_len + 1);
    strncat(query, "&", 2);

    strncat(sig_base, "api_key", 8);
    strncat(sig_base, escaped_api_key, escaped_key_len + 1);
    curl_free(escaped_api_key);

    const char *method = API_METHOD_GET_TOKEN;
    size_t method_len = strlen(method);
    strncat(query, "method=", 8);
    strncat(query, method, method_len + 1);
    strncat(query, "&", 2);

    strncat(sig_base, "method", 7);
    strncat(sig_base, method, method_len + 1);

    char *sig = (char *) api_get_signature(sig_base, secret);
    strncat(query, "api_sig=", 9);
    strncat(query, sig, MAX_PROPERTY_LENGTH);
    strncat(query, "&", 2);
    string_free(sig);
    string_free(sig_base);

    strncat(query, "format=json", 12);
    curl_easy_cleanup(handle);

    request->request_type = http_post;
    request->query = query;
    request->end_point = api_endpoint_new(auth);
    request->url = api_get_url(request->end_point);
    return request;
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
struct http_request *audioscrobbler_api_build_request_get_session(const struct api_credentials *auth)
{
    if (!audioscrobbler_valid_credentials(auth)) { return NULL; }
    CURL *handle = curl_easy_init();

    const char *api_key = auth->api_key;
    const char *secret = auth->secret;
    const char *token = auth->token;

    struct http_request *request = http_request_new();

    const char *method = API_METHOD_GET_SESSION;

    char *sig_base = get_zero_string(MAX_BODY_SIZE);
    char *query = get_zero_string(MAX_BODY_SIZE);

    char *escaped_api_key = curl_easy_escape(handle, api_key, strlen(api_key));
    size_t escaped_key_len = strlen(escaped_api_key);
    strncat(query, "api_key=", 9);
    strncat(query, escaped_api_key, escaped_key_len + 1);
    strncat(query, "&", 2);

    strncat(sig_base, "api_key", 8);
    strncat(sig_base, escaped_api_key, escaped_key_len + 1);
    curl_free(escaped_api_key);

    size_t method_len = strlen(method);
    strncat(query, "method=", 8);
    strncat(query, method, method_len + 1);
    strncat(query, "&", 2);

    strncat(sig_base, "method", 7);
    strncat(sig_base, method, method_len + 1);

    size_t token_len = strlen(token);
    strncat(query, "token=", 7);
    strncat(query, token, token_len + 1);
    strncat(query, "&", 2);

    strncat(sig_base, "token", 6);
    strncat(sig_base, token, token_len + 1);

    char *sig = (char *) api_get_signature(sig_base, secret);
    strncat(query, "api_sig=", 9);
    strncat(query, sig, MAX_PROPERTY_LENGTH);
    strncat(query, "&", 2);
    string_free(sig);
    string_free(sig_base);

    strncat(query, "format=json", 12);
    curl_easy_cleanup(handle);

    request->request_type = http_post;
    request->query = query;
    request->end_point = api_endpoint_new(auth);
    request->url = api_get_url(request->end_point);
    return request;
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
struct http_request *audioscrobbler_api_build_request_now_playing(const struct scrobble *tracks[], const int track_count, const struct api_credentials *auth)
{
    CURL *handle = curl_easy_init();
    if (!audioscrobbler_valid_credentials(auth)) { return NULL; }

    assert(track_count == 1);

    const struct scrobble *track = tracks[0];
    const char *api_key = auth->api_key;
    const char *secret = auth->secret;
    const char *sk = auth->session_key;

    struct http_request *request = http_request_new();

    char *sig_base = get_zero_string(MAX_BODY_SIZE);
    char *body = get_zero_string(MAX_BODY_SIZE);

    assert(track->album);
    size_t album_len = strlen(track->album);
    char *esc_album = curl_easy_escape(handle, track->album, album_len);
    size_t esc_album_len = strlen(esc_album);
    strncat(body, "album=", 7);
    strncat(body, esc_album, esc_album_len + 1);
    strncat(body, "&", 2);

    strncat(sig_base, "album", 6);
    strncat(sig_base, track->album, album_len + 1);
    curl_free(esc_album);

    assert(api_key);
    size_t api_key_len = strlen(api_key);
    char *esc_api_key = curl_easy_escape(handle, api_key, api_key_len);
    size_t esc_api_key_len = strlen(esc_api_key);
    strncat(body, "api_key=", 9);
    strncat(body, esc_api_key, esc_api_key_len + 1);
    strncat(body, "&", 2);

    strncat(sig_base, "api_key", 8);
    strncat(sig_base, api_key, api_key_len + 1);
    curl_free(esc_api_key);

    char full_artist[MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT] = {0};
    size_t full_artist_len = 0;
    for (size_t i = 0; i < array_count(track->artist); i++) {
        const char *artist = track->artist[i];
        size_t artist_len = strlen(artist);
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
        char *esc_full_artist = curl_easy_escape(handle, full_artist, full_artist_len);
        size_t esc_full_artist_len = strlen(esc_full_artist);
        size_t artist_label_len = strlen(API_ARTIST_NODE_NAME);

        strncat(body, API_ARTIST_NODE_NAME "=", artist_label_len + 2);
        strncat(body, esc_full_artist, esc_full_artist_len + 1);
        strncat(body, "&", 2);

        strncat(sig_base, API_ARTIST_NODE_NAME, artist_label_len + 1);
        strncat(sig_base, full_artist, full_artist_len);
        curl_free(esc_full_artist);
    }

    char *mb_track_id = (char *) track->mb_track_id[0];
    size_t mbid_len = strlen(mb_track_id);
    if (mbid_len > 0) {
        char *esc_mbid = curl_easy_escape(handle, mb_track_id, mbid_len);
        size_t esc_mbid_len = strlen(esc_mbid);
        size_t mbid_label_len = strlen(API_MUSICBRAINZ_MBID_NODE_NAME);

        strncat(body, API_MUSICBRAINZ_MBID_NODE_NAME "=", mbid_label_len + 2);
        strncat(body, esc_mbid, esc_mbid_len + 1);
        strncat(body, "&", 2);

        strncat(sig_base, API_MUSICBRAINZ_MBID_NODE_NAME, mbid_label_len + 1);
        strncat(sig_base, mb_track_id, mbid_len +1);
        curl_free(esc_mbid);
    }

    const char *method = API_METHOD_NOW_PLAYING;

    assert(method);
    size_t method_len = strlen(method);
    strncat(body, "method=", 8);
    strncat(body, method, method_len + 1);
    strncat(body, "&", 2);

    strncat(sig_base, "method", 7);
    strncat(sig_base, method, method_len + 1);

    assert(sk);
    strncat(body, "sk=", 4);
    strncat(body, sk, MAX_PROPERTY_LENGTH);
    strncat(body, "&", 2);

    strncat(sig_base, "sk", 3);
    strncat(sig_base, sk, MAX_PROPERTY_LENGTH);

    assert(track->title);
    size_t title_len = strlen(track->title);
    char *esc_title = curl_easy_escape(handle, track->title, title_len);
    size_t esc_title_len = strlen(esc_title);
    strncat(body, "track=", 7);
    strncat(body, esc_title, esc_title_len + 1);
    strncat(body, "&", 2);

    strncat(sig_base, "track", 6);
    strncat(sig_base, track->title, title_len + 1);
    curl_free(esc_title);

    char *sig = (char *) api_get_signature(sig_base, secret);
    strncat(body, "api_sig=", 9);
    strncat(body, sig, MAX_PROPERTY_LENGTH);
    string_free(sig_base);
    string_free(sig);

    char *query = get_zero_string(MAX_BODY_SIZE);
    strncat(query, "format=json", 12);
    curl_easy_cleanup(handle);

    request->request_type = http_post;
    request->query = query;
    request->body = body;
    request->body_length = strlen(body);
    request->end_point = api_endpoint_new(auth);
    request->url = api_get_url(request->end_point);
    return request;
}

struct http_request *audioscrobbler_api_build_request_scrobble(const struct scrobble *tracks[], const int track_count, const struct api_credentials *auth)
{
    if (!audioscrobbler_valid_credentials(auth)) { return NULL; }
    CURL *handle = curl_easy_init();

    const char *api_key = auth->api_key;
    const char *secret = auth->secret;
    const char *sk = auth->session_key;

    struct http_request *request = http_request_new();

    const char *method = API_METHOD_SCROBBLE;

    char *sig_base = get_zero_string(MAX_BODY_SIZE);
    char *body = get_zero_string(MAX_BODY_SIZE);

    for (int i = 0; i < track_count; i++) {
        const struct scrobble *track = tracks[i];
        assert(track->album);

        size_t album_len = strlen(track->album);
        assert(album_len != 0);

        char *esc_album = curl_easy_escape(handle, track->album, album_len);
        char *album_body = get_zero_string(MAX_PROPERTY_LENGTH);
        snprintf(album_body, MAX_PROPERTY_LENGTH, API_ALBUM_NODE_NAME "[%d]=%s&", i, esc_album);
        strncat(body, album_body, MAX_PROPERTY_LENGTH);

        char *album_sig = get_zero_string(MAX_PROPERTY_LENGTH);
        snprintf(album_sig, MAX_PROPERTY_LENGTH + 18, API_ALBUM_NODE_NAME "[%d]%s", i, track->album);
        strncat(sig_base, album_sig, MAX_PROPERTY_LENGTH);

        curl_free(esc_album);
        string_free(album_sig);
        string_free(album_body);
    }

    assert(api_key);
    size_t api_key_len = strlen(api_key);
    char *esc_api_key = curl_easy_escape(handle, api_key, api_key_len);
    size_t esc_api_key_len = strlen(esc_api_key);
    strncat(body, "api_key=", 9);
    strncat(body, esc_api_key, esc_api_key_len + 1);
    strncat(body, "&", 2);

    strncat(sig_base, "api_key", 8);
    strncat(sig_base, api_key, api_key_len + 1);
    curl_free(esc_api_key);

    for (int i = 0; i < track_count; i++) {
        const struct scrobble *track = tracks[i];

        char full_artist[MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT] = {0};
        size_t full_artist_len = 0;
        for (unsigned j = 0; j < array_count(track->artist); j++) {
            const char *artist = track->artist[j];
            size_t artist_len = strlen(artist);
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
            char *esc_full_artist = curl_easy_escape(handle, full_artist, full_artist_len);

            const char *fmt_full_artist = API_ARTIST_NODE_NAME "[%d]=%s&";
            size_t fmt_full_artist_len = strlen(fmt_full_artist) - 4;

            size_t artist_body_len = MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT + fmt_full_artist_len;

            char artist_body[artist_body_len + 1];
            snprintf(artist_body, artist_body_len, fmt_full_artist, i, esc_full_artist);

            strncat(body, artist_body, artist_body_len);

            const char *fmt_artist_sig = API_ARTIST_NODE_NAME "[%d]%s";
            size_t fmt_artist_sig_len = strlen(fmt_full_artist) + 4;
            size_t artist_sig_len = MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT + fmt_artist_sig_len;

            char artist_sig[artist_sig_len];
            snprintf(artist_sig, artist_sig_len, fmt_artist_sig, i, full_artist);

            strncat(sig_base, artist_sig, artist_sig_len);
            curl_free(esc_full_artist);
        }
    }

    for (int i = 0; i < track_count; i++) {
        const struct scrobble *track = tracks[i];
        assert(track->mb_track_id);
        char *mb_track_id = (char *) track->mb_track_id[0];
        size_t mbid_len = strlen(mb_track_id);
        if (mbid_len > 0) {
            char *esc_mbid = curl_easy_escape(handle, mb_track_id, mbid_len);

            char *mbid_body = get_zero_string(MAX_PROPERTY_LENGTH);
            snprintf(mbid_body, MAX_PROPERTY_LENGTH, API_MUSICBRAINZ_MBID_NODE_NAME "[%d]=%s&", i, esc_mbid);
            strncat(body, mbid_body, MAX_PROPERTY_LENGTH);

            char *mbid_sig = get_zero_string(MAX_PROPERTY_LENGTH);
            snprintf(mbid_sig, MAX_PROPERTY_LENGTH + 17, API_MUSICBRAINZ_MBID_NODE_NAME "[%d]%s", i, mb_track_id);
            strncat(sig_base, mbid_sig, MAX_PROPERTY_LENGTH);

            curl_free(esc_mbid);
            string_free(mbid_sig);
            string_free(mbid_body);
        }
    }

    assert(method);
    size_t method_len = strlen(method);
    strncat(body, "method=", 8);
    strncat(body, method, method_len + 1);
    strncat(body, "&", 2);

    strncat(sig_base, "method", 7);
    strncat(sig_base, method, method_len + 1);

    assert(sk);
    strncat(body, "sk=", 4);
    strncat(body, sk, MAX_PROPERTY_LENGTH);
    strncat(body, "&", 2);

    strncat(sig_base, "sk", 3);
    strncat(sig_base, sk, MAX_PROPERTY_LENGTH);

    for (int i = track_count - 1; i >= 0; i--) {
        const struct scrobble *track = tracks[i];

        char *tstamp_body = get_zero_string(MAX_PROPERTY_LENGTH);
        snprintf(tstamp_body, MAX_PROPERTY_LENGTH, API_TIMESTAMP_NODE_NAME "[%d]=%ld&", i, track->start_time);
        strncat(body, tstamp_body, MAX_PROPERTY_LENGTH);

        char *tstamp_sig = get_zero_string(MAX_PROPERTY_LENGTH);
        snprintf(tstamp_sig, MAX_PROPERTY_LENGTH, API_TIMESTAMP_NODE_NAME "[%d]%ld", i, track->start_time);
        strncat(sig_base, tstamp_sig, MAX_PROPERTY_LENGTH);

        string_free(tstamp_sig);
        string_free(tstamp_body);
    }

    for (int i = track_count - 1; i >= 0; i--) {
        const struct scrobble *track = tracks[i];

        assert(track->title);

        size_t title_len = strlen(track->title);
        assert(title_len != 0);

        char *esc_title = curl_easy_escape(handle, track->title, title_len);

        char *title_body = get_zero_string(MAX_PROPERTY_LENGTH);
        snprintf(title_body, MAX_PROPERTY_LENGTH, API_TRACK_NODE_NAME "[%d]=%s&", i, esc_title);
        strncat(body, title_body, MAX_PROPERTY_LENGTH);

        char *title_sig = get_zero_string(MAX_PROPERTY_LENGTH);
        snprintf(title_sig, MAX_PROPERTY_LENGTH + 18, API_TRACK_NODE_NAME "[%d]%s", i, track->title);
        strncat(sig_base, title_sig, MAX_PROPERTY_LENGTH);

        curl_free(esc_title);
        string_free(title_sig);
        string_free(title_body);
    }

    char *sig = (char *) api_get_signature(sig_base, secret);
    strncat(body, "api_sig=", 9);
    strncat(body, sig, MAX_PROPERTY_LENGTH);
    string_free(sig);
    string_free(sig_base);
    curl_easy_cleanup(handle);

    char *query = get_zero_string(MAX_BODY_SIZE);
    strncat(query, "format=json", 12);

    request->request_type = http_post;
    request->query = query;
    request->body = body;
    request->body_length = strlen(body);
    request->end_point = api_endpoint_new(auth);
    request->url = api_get_url(request->end_point);

    return request;
}

#endif // MPRIS_SCROBBLER_AUDIOSCROBBLER_API_H
