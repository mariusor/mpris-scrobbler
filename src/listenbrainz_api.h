/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_LISTENBRAINZ_API_H
#define MPRIS_SCROBBLER_LISTENBRAINZ_API_H

#ifndef LISTENBRAINZ_API_KEY
#include "credentials_listenbrainz.h"
#endif


#define API_LISTEN_TYPE_NOW_PLAYING     "playing_now"
#define API_LISTEN_TYPE_SCROBBLE        "single"

#define API_LISTEN_TYPE_NODE_NAME       "listen_type"
#define API_PAYLOAD_NODE_NAME           "payload"
#define API_LISTENED_AT_NODE_NAME       "listened_at"
#define API_METADATA_NODE_NAME          "track_metadata"
#define API_ARTIST_NAME_NODE_NAME       "artist_name"
#define API_TRACK_NAME_NODE_NAME        "track_name"
#define API_ALBUM_NAME_NODE_NAME        "release_name"

#define API_ENDPOINT_SUBMIT_LISTEN      "/1/submit-listens"
#define API_HEADER_AUTHORIZATION_TOKENIZED "Authorization: %s"


static bool listenbrainz_valid_credentials(const struct api_credentials *auth)
{
    bool status = false;
    if (NULL == auth) { return status; }
    if (auth->end_point != listenbrainz) { return status; }

    status = true;
    return status;
}

#if 0
static http_request *build_generic_request()
{
}
#endif

void print_http_request(struct http_request*);
/*
 */
struct http_request *listenbrainz_api_build_request_now_playing(const struct scrobble *track, CURL *handle, const struct api_credentials *auth)
{
    if (NULL == handle) { return NULL; }
    if (!listenbrainz_valid_credentials(auth)) { return NULL; }

    const char *token = auth->token;

    char *body = get_zero_string(MAX_BODY_SIZE);
    char *query = get_zero_string(MAX_BODY_SIZE);
    strncpy(query, API_ENDPOINT_SUBMIT_LISTEN, strlen(API_ENDPOINT_SUBMIT_LISTEN));

    char *authorization_header = get_zero_string(MAX_URL_LENGTH);
    snprintf(authorization_header, MAX_URL_LENGTH, API_HEADER_AUTHORIZATION_TOKENIZED, token);

    struct http_request *request = http_request_new();
    request->headers[0] = authorization_header;
    request->headers_count++;

    request->query = query;
    request->body = body;
    request->body_length = strlen(body);
    request->end_point = api_endpoint_new(auth->end_point);
    request->url = api_get_url(request->end_point);
    print_http_request(request);

    return request;
}

/*
 */
struct http_request *listenbrainz_api_build_request_scrobble(const struct scrobble *tracks[], size_t track_count, CURL *handle, const struct api_credentials *auth)
{
    if (NULL == handle) { return NULL; }
    if (!listenbrainz_valid_credentials(auth)) { return NULL; }

    const char *token = auth->token;

    char *authorization_header = get_zero_string(MAX_URL_LENGTH);
    snprintf(authorization_header, MAX_URL_LENGTH, API_HEADER_AUTHORIZATION_TOKENIZED, token);

    char *body = get_zero_string(MAX_BODY_SIZE);
    char *query = get_zero_string(MAX_BODY_SIZE);

    struct http_request *request = http_request_new();
    request->headers[0] = authorization_header;
    request->headers_count++;

    request->query = query;
    request->body = body;
    request->body_length = strlen(body);
    request->end_point = api_endpoint_new(auth->end_point);
    request->url = api_get_url(request->end_point);

    print_http_request(request);
    return request;
}

#endif // MPRIS_SCROBBLER_LISTENBRAINZ_API_H
