/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#ifndef MPRIS_SCROBBLER_LISTENBRAINZ_API_H
#define MPRIS_SCROBBLER_LISTENBRAINZ_API_H

#ifndef LISTENBRAINZ_API_KEY
#include "credentials_listenbrainz.h"
#endif


#define API_LISTEN_TYPE_NOW_PLAYING     "playing_now"
#define API_LISTEN_TYPE_SINGLE          "single"
#define API_LISTEN_TYPE_IMPORT          "import"

#define API_LISTEN_TYPE_NODE_NAME       "listen_type"
#define API_PAYLOAD_NODE_NAME           "payload"
#define API_LISTENED_AT_NODE_NAME       "listened_at"
#define API_METADATA_NODE_NAME          "track_metadata"
#define API_ARTIST_NAME_NODE_NAME       "artist_name"
#define API_TRACK_NAME_NODE_NAME        "track_name"
#define API_ALBUM_NAME_NODE_NAME        "release_name"

#define API_ADDITIONAL_INFO_NODE_NAME             "additional_info"
#define API_MUSICBRAINZ_TRACK_ID_NODE_NAME        "track_mbid"
#define API_MUSICBRAINZ_RECORDING_ID_NODE_NAME    "recording_mbid"
#define API_MUSICBRAINZ_ARTISTS_ID_NODE_NAME      "artist_mbids"
#define API_MUSICBRAINZ_ALBUM_ID_NODE_NAME        "release_mbid"
#define API_MUSICBRAINZ_SPOTIFY_ID_NODE_NAME      "spotify_id"

#define API_CODE_NODE_NAME              "code"
#define API_ERROR_NODE_NAME             "error"

#define LISTENBRAINZ_AUTH_URL           "https://listenbrainz.org/api/auth/?api_key=%s&token=%s"
#define LISTENBRAINZ_API_BASE_URL       "api.listenbrainz.org"
#define LISTENBRAINZ_API_VERSION        "1"

#define API_ENDPOINT_SUBMIT_LISTEN      "submit-listens"

static bool listenbrainz_valid_credentials(const struct api_credentials *auth)
{
    bool status = false;
    if (NULL == auth) { return status; }
    if (auth->end_point != api_listenbrainz) { return status; }

    status = true;
    return status;
}

#if 0
static http_request *build_generic_request()
{
}
#endif

struct http_header *http_authorization_header_new (const char*);
struct http_header *http_content_type_header_new (void);
struct http_request *listenbrainz_api_build_request_now_playing(const struct scrobble *tracks[], const int track_count, const struct api_credentials *auth)
{
    if (!listenbrainz_valid_credentials(auth)) { return NULL; }

    assert(track_count == 1);

    const struct scrobble *track = tracks[0];

    const char *token = auth->token;

    char *body = get_zero_string(MAX_BODY_SIZE);
    if (NULL == body) { return NULL; }

    json_object *root = json_object_new_object();
    json_object_object_add(root, API_LISTEN_TYPE_NODE_NAME, json_object_new_string(API_LISTEN_TYPE_NOW_PLAYING));

    json_object *payload = json_object_new_array();

    json_object *payload_elem = json_object_new_object();
    json_object *metadata = json_object_new_object();
    json_object_object_add(metadata, API_ALBUM_NAME_NODE_NAME, json_object_new_string(track->album));

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
        strncat(full_artist, artist, MAX_PROPERTY_LENGTH);
        full_artist_len += artist_len;
    }
    if (full_artist_len > 0) {
        json_object_object_add(metadata, API_ARTIST_NAME_NODE_NAME, json_object_new_string(full_artist));
    }
    json_object_object_add(metadata, API_TRACK_NAME_NODE_NAME, json_object_new_string(track->title));

    char *mb_track_id = (char*)track->mb_track_id[0];
    char *mb_artist_id = (char*)track->mb_artist_id[0];
    char *mb_album_id = (char*)track->mb_album_id[0];
    if (strlen(mb_track_id) > 0 || strlen(mb_artist_id) > 0 || strlen(mb_album_id) > 0 || strlen(track->mb_spotify_id) > 0) {
        json_object *additional_info = json_object_new_object();

        if (strlen(mb_track_id) > 0) {
            json_object_object_add(additional_info, API_MUSICBRAINZ_RECORDING_ID_NODE_NAME, json_object_new_string(mb_track_id));
        }
        if (strlen(mb_artist_id) > 0) {
            json_object *artists_ids = json_object_new_array();
            json_object_array_add(artists_ids, json_object_new_string(mb_artist_id));
            json_object_object_add(additional_info, API_MUSICBRAINZ_ARTISTS_ID_NODE_NAME, artists_ids);
        }
        if (strlen(mb_album_id) > 0) {
            json_object_object_add(additional_info, API_MUSICBRAINZ_ALBUM_ID_NODE_NAME, json_object_new_string(mb_album_id));
        }
        if (strlen(track->mb_spotify_id) > 0) {
            json_object_object_add(additional_info, API_MUSICBRAINZ_SPOTIFY_ID_NODE_NAME, json_object_new_string(track->mb_spotify_id));
        }

        json_object_object_add(metadata, API_ADDITIONAL_INFO_NODE_NAME, additional_info);
    }
    json_object_object_add(payload_elem, API_METADATA_NODE_NAME, metadata);

    json_object_array_add(payload, payload_elem);
    json_object_object_add(root, API_PAYLOAD_NODE_NAME, payload);

    const char *json_str = json_object_to_json_string(root);
    strncpy(body, json_str, MAX_BODY_SIZE);

    struct http_request *request = http_request_new();
    arrput(request->headers, http_authorization_header_new(token));
    arrput(request->headers, http_content_type_header_new());

    request->request_type = http_post;
    request->body = body;
    request->body_length = strlen(body);
    request->end_point = api_endpoint_new(auth);
    request->url = api_get_url(request->end_point);
    strncpy(request->url + strlen(request->url), API_ENDPOINT_SUBMIT_LISTEN, strlen(API_ENDPOINT_SUBMIT_LISTEN) + 1);

    json_object_put(root);

    return request;
}

/*
 */
struct http_request *listenbrainz_api_build_request_scrobble(const struct scrobble *tracks[], const int track_count, const struct api_credentials *auth)
{
    if (!listenbrainz_valid_credentials(auth)) { return NULL; }

    const char *token = auth->token;

    char *body = get_zero_string(MAX_BODY_SIZE);
    if (NULL == body) { return NULL; }
    char *query = get_zero_string(MAX_BODY_SIZE);
    if (NULL == query) { return NULL; }

    json_object *root = json_object_new_object();
    if (track_count > 1) {
        json_object_object_add(root, API_LISTEN_TYPE_NODE_NAME, json_object_new_string(API_LISTEN_TYPE_IMPORT));
    } else {
        json_object_object_add(root, API_LISTEN_TYPE_NODE_NAME, json_object_new_string(API_LISTEN_TYPE_SINGLE));
    }

    json_object *payload = json_object_new_array();
    for (int ti = 0; ti < track_count; ti++) {
        const struct scrobble *track = tracks[ti];
        json_object *payload_elem = json_object_new_object();
        json_object *metadata = json_object_new_object();
        if (strlen(track->album) > 0) {
            json_object_object_add(metadata, API_ALBUM_NAME_NODE_NAME, json_object_new_string(track->album));
        }
        char full_artist[MAX_PROPERTY_LENGTH * MAX_PROPERTY_COUNT] = {0};
        size_t full_artist_len = 0;
        for (size_t ai = 0; ai < array_count(track->artist); ai++) {
            const char *artist = track->artist[ai];
            size_t artist_len = strlen(artist);
            if (NULL == artist || artist_len == 0) { continue; }

            if (full_artist_len > 0) {
                size_t l_val_sep = strlen(VALUE_SEPARATOR);
                strncat(full_artist, VALUE_SEPARATOR, l_val_sep + 1);
                full_artist_len += l_val_sep;
            }
            strncat(full_artist, artist, MAX_PROPERTY_LENGTH);
            full_artist_len += artist_len;
        }
        if (full_artist_len > 0) {
            json_object_object_add(metadata, API_ARTIST_NAME_NODE_NAME, json_object_new_string(full_artist));
        }
        if (strlen(track->title) > 0) {
            json_object_object_add(metadata, API_TRACK_NAME_NODE_NAME, json_object_new_string(track->title));
        }
        char *mb_track_id = (char*)track->mb_track_id[0];
        char *mb_artist_id = (char*)track->mb_artist_id[0];
        char *mb_album_id = (char*)track->mb_album_id[0];
        if ( (strlen(mb_track_id) > 0)|| (strlen(mb_artist_id) > 0) || (strlen(mb_album_id) > 0) || (strlen(track->mb_spotify_id) > 0)) {
            json_object *additional_info = json_object_new_object();
            if (strlen(mb_track_id) > 0) {
                json_object_object_add(additional_info, API_MUSICBRAINZ_RECORDING_ID_NODE_NAME, json_object_new_string(mb_track_id));
            }
            if (strlen(mb_artist_id) > 0) {
                json_object *artists_ids = json_object_new_array();
                json_object_array_add(artists_ids, json_object_new_string(mb_artist_id));
                json_object_object_add(additional_info, API_MUSICBRAINZ_ARTISTS_ID_NODE_NAME, artists_ids);
            }
            if (strlen(mb_album_id) > 0) {
                json_object_object_add(additional_info, API_MUSICBRAINZ_ALBUM_ID_NODE_NAME, json_object_new_string(mb_album_id));
            }
            if (strlen(track->mb_spotify_id) > 0) {
                json_object_object_add(additional_info, API_MUSICBRAINZ_SPOTIFY_ID_NODE_NAME, json_object_new_string(track->mb_spotify_id));
            }

            json_object_object_add(metadata, API_ADDITIONAL_INFO_NODE_NAME, additional_info);
        }

        json_object_object_add(payload_elem, API_LISTENED_AT_NODE_NAME, json_object_new_int64(track->start_time));
        json_object_object_add(payload_elem, API_METADATA_NODE_NAME, metadata);

        json_object_array_add(payload, payload_elem);
    }

    json_object_object_add(root, API_PAYLOAD_NODE_NAME, payload);

    const char *json_str = json_object_to_json_string(root);
    strncpy(body, json_str, MAX_BODY_SIZE);

    struct http_request *request = http_request_new();
    arrput(request->headers, (http_authorization_header_new(token)));
    arrput(request->headers, (http_content_type_header_new()));

    request->request_type = http_post;
    request->query = query;
    request->body = body;
    request->body_length = strlen(body);
    request->end_point = api_endpoint_new(auth);
    request->url = api_get_url(request->end_point);
    strncpy(request->url + strlen(request->url), API_ENDPOINT_SUBMIT_LISTEN, strlen(API_ENDPOINT_SUBMIT_LISTEN) + 1);

    json_object_put(root);

    return request;
}

bool listenbrainz_json_document_is_error(const char *buffer, const size_t length)
{
    // { "code": 401, "error": "You need to provide an Authorization header." }
    bool result = false;

    if (NULL == buffer) { return result; }
    if (length == 0) { return result; }

    json_object *code_object = NULL;
    json_object *err_object = NULL;

    struct json_tokener *tokener = json_tokener_new();
    if (NULL == tokener) { return result; }
    json_object *root = json_tokener_parse_ex(tokener, buffer, length);

    if (NULL == root || json_object_object_length(root) < 1) {
        goto _exit;
    }
    json_object_object_get_ex(root, API_CODE_NODE_NAME, &code_object);
    json_object_object_get_ex(root, API_ERROR_NODE_NAME, &err_object);
    if (NULL == code_object || !json_object_is_type(code_object, json_type_string)) {
        goto _exit;
    }
    if (NULL == err_object || !json_object_is_type(err_object, json_type_int)) {
        goto _exit;
    }
    result = true;

_exit:
    if (NULL != root) { json_object_put(root); }
    json_tokener_free(tokener);
    return result;
}

#endif // MPRIS_SCROBBLER_LISTENBRAINZ_API_H
