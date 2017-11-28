/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_API_H
#define MPRIS_SCROBBLER_API_H

#include <inttypes.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <openssl/md5.h>

#define MAX_HEADERS                     10
#define MAX_HEADER_LENGTH               256
#define MAX_URL_LENGTH                  2048
#define MAX_BODY_SIZE                   16384

#define CONTENT_TYPE_XML            "application/xml"
#define CONTENT_TYPE_JSON           "application/json"

enum api_return_status {
    status_failed  = 0, // failed == bool false
    status_ok           // ok == bool true
};

struct api_response {
    enum api_return_status status;
    struct api_error *error;
};

typedef enum message_types {
    api_call_authenticate,
    api_call_now_playing,
    api_call_scrobble,
} message_type;

struct http_header {
    char *name;
    char *value;
};

struct http_response {
    unsigned short code;
    char *body;
    size_t body_length;
    //struct http_header *headers[MAX_HEADERS];
    char *headers[MAX_HEADERS];
    size_t headers_count;
};

typedef enum http_request_types {
    http_get,
    http_post,
    http_put,
    http_head,
    http_patch,
} http_request_type;

struct api_endpoint {
    char *scheme;
    char *host;
    char *path;
};

struct http_request {
    http_request_type request_type;
    struct api_endpoint *end_point;
    char *url;
    char *query;
    char *body;
    size_t body_length;
    char *headers[MAX_HEADERS];
    size_t headers_count;
};

#include "audioscrobbler_api.h"
#include "listenbrainz_api.h"

#if 0
#define HTTP_HEADER_CONTENT_TYPE "ContentType"
char *http_response_headers_content_type(struct http_response *res)
{

    for (size_t i = 0; i < res->headers_count; i++) {
        struct http_header *current = res->headers[i];

        if(strncmp(current->name, HTTP_HEADER_CONTENT_TYPE, strlen(HTTP_HEADER_CONTENT_TYPE)) == 0) {
             return current->value;
        }
    }
    return NULL;
}

static void http_response_parse_json_body(struct http_response *res)
{
    if (NULL == res) { return; }
    if (res->code >= 500) { return; }

    json_object *obj = json_tokener_parse(res->body);
    if (NULL == obj) {
        _warn("json::parse_error");
    }
    if (json_object_array_length(obj) < 1) {
        _warn("json::invalid_json_message");
    }
    _trace("json::obj_cnt: %d", json_object_array_length(obj));
#if 0
    if (json_object_type(object, json_type_object)) {
        json_object_object_get_ex();
    }
#endif
}
#endif

char *api_get_url(struct api_endpoint *endpoint)
{
    if (NULL == endpoint) { return NULL; }
    char *url = get_zero_string(MAX_URL_LENGTH);

    strncat(url, endpoint->scheme, strlen(endpoint->scheme));
    strncat(url, "://", 3);
    strncat(url, endpoint->host, strlen(endpoint->host));
    strncat(url, endpoint->path, strlen(endpoint->path));

    return url;
}

char *http_request_get_url(const struct http_request *request)
{
    if (NULL == request) { return NULL; }
    char *url = get_zero_string(MAX_URL_LENGTH);

    strncat(url, request->url, strlen(request->url));
    if (NULL != request->query) {
        strncat(url, "?", 1);
        strncat(url, request->query, strlen(request->query));
    }

    return url;
}

void api_endpoint_free(struct api_endpoint *api)
{
    free(api);
}

struct api_endpoint *api_endpoint_new(enum api_type type)
{
    struct api_endpoint *result = malloc(sizeof(struct api_endpoint));
    result->scheme = "https";
    switch (type) {
        case lastfm:
            result->host = LASTFM_API_BASE_URL;
            result->path = "/" LASTFM_API_VERSION "/";
            break;
        case librefm:
            result->host = LIBREFM_API_BASE_URL;
            result->path = "/" LIBREFM_API_VERSION "/";
            break;
        case listenbrainz:
            result->host = LISTENBRAINZ_API_BASE_URL;
            result->path = "/" LISTENBRAINZ_API_VERSION "/";
            break;
        case unknown:
        default:
            break;
    }

    return result;
}

#if 0
static const char *get_api_error_message(api_return_code code)
{
    switch (code) {
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

void http_request_free(struct http_request *req)
{
    if (NULL == req) { return; }
    if (NULL != req->body) { free(req->body); }
    if (NULL != req->query) { free(req->query); }
    if (NULL != req->url) { free(req->url); }
    for (size_t i = 0; i < req->headers_count; i++) {
        if (NULL != req->headers[i]) { free(req->headers[i]); }
    }
    api_endpoint_free(req->end_point);
    free(req);
}

#if 0
static struct http_header *http_header_new(void)
{
    struct http_header *header = malloc(sizeof(struct http_header));
    header->name = get_zero_string(MAX_HEADER_LENGTH);
    header->value = get_zero_string(MAX_HEADER_LENGTH);
    return header;
}
#endif

struct http_request *http_request_new(void)
{
    struct http_request *req = malloc(sizeof(struct http_request));
    req->url = NULL;
    req->body = NULL;
    req->body_length = 0;
    req->query = NULL;
    req->end_point = NULL;
    req->headers[0] = NULL;
    req->headers_count = 0;

    return req;
}

void print_http_request(struct http_request *req)
{
    char *url = http_request_get_url(req);
    _trace("http::req[%p]%s: %s", req, (req->request_type == http_get ? "GET" : "POST"), url);
    if (req->headers_count > 0) {
        _trace("http::req::headers[%lu]:", req->headers_count);
        for (size_t i = 0; i < req->headers_count; i++) {
            _trace("\theader[%lu]: %s", i, req->headers[i]);
        }
    }
    if (req->request_type != http_get) {
        _trace("http::req[%lu]: %s", req->body_length, req->body);
    }
    free(url);
}

void print_http_response(struct http_response *resp)
{
    _trace("http::resp[%p]: %u", resp, resp->code);
    if (resp->headers_count > 0) {
        _trace("http::resp::headers[%lu]:", resp->headers_count);
        for (size_t i = 0; i < resp->headers_count; i++) {
            _trace("\theader[%lu]: %s", i, resp->headers[i]);
        }
    }
}

char *api_get_signature(const char *string, const char *secret)
{
    size_t len = strlen(string) + strlen(secret);
    char *sig = get_zero_string(len);
    snprintf(sig, len + 1, "%s%s", string, secret);

    unsigned char sig_hash[MD5_DIGEST_LENGTH];
    MD5_CTX h;
    MD5_Init(&h);
    MD5_Update(&h, sig, len);
    MD5_Final(sig_hash, &h);
    free(sig);

    char *result = get_zero_string(MD5_DIGEST_LENGTH * 2 + 2);
    for (size_t n = 0; n < MD5_DIGEST_LENGTH; n++) {
        snprintf(result + 2 * n, 3, "%02x", sig_hash[n]);
    }

    return result;
}

bool credentials_valid(struct api_credentials *c)
{
    switch (c->end_point) {
        case librefm:
#ifndef LIBREFM_API_SECRET
            return false;
#endif
            break;
        case lastfm:
#ifndef LASTFM_API_SECRET
            return false;
#endif
            break;
        case listenbrainz:
#ifndef LISTENBRAINZ_API_SECRET
            return false;
#endif
            break;
        case unknown:
        default:
            return false;
            break;
    }
    return (c->enabled && c->authenticated);
}

const char *api_get_application_secret(enum api_type type)
{
    switch (type) {
#ifdef LIBREFM_API_SECRET
        case librefm:
            return LIBREFM_API_SECRET;
            break;
#endif
#ifdef LASTFM_API_SECRET
        case lastfm:
            return LASTFM_API_SECRET;
            break;
#endif
#ifdef LISTENBRAINZ_API_SECRET
        case listenbrainz:
            return LISTENBRAINZ_API_SECRET;
#endif
            break;
        default:
            return NULL;
    }
    return NULL;
}

const char *api_get_application_key(enum api_type type)
{
    switch (type) {
#ifdef LIBREFM_API_KEY
        case librefm:
            return LIBREFM_API_KEY;
            break;
#endif
#ifdef LASTFM_API_KEY
        case lastfm:
            return LASTFM_API_KEY;
            break;
#endif
#ifdef LISTENBRAINZ_API_KEY
        case listenbrainz:
            return LISTENBRAINZ_API_KEY;
            break;
#endif
        default:
            return NULL;
    }
    return NULL;
}

char *api_get_auth_url(enum api_type type, const char *token)
{
    if (NULL == token) { return NULL; }

    const char *base_url = NULL;
    switch(type) {
        case lastfm:
            base_url = LASTFM_AUTH_URL;
            break;
        case librefm:
            base_url = LIBREFM_AUTH_URL;
            break;
        case listenbrainz:
            base_url = LISTENBRAINZ_AUTH_URL;
            break;
        case unknown:
        default:
            return NULL;
    }
    const char *api_key = api_get_application_key(type);
    size_t token_len = strlen(token);
    size_t key_len = strlen(api_key);
    size_t base_url_len = strlen(base_url);

    size_t url_len = base_url_len + token_len + key_len;
    char *url = get_zero_string(url_len);

    snprintf(url, url_len, base_url, api_key, token);

    return url;
}

struct http_request *api_build_request_get_token(CURL *handle, const struct api_credentials *auth)
{
    switch (auth->end_point) {
        case listenbrainz:
            break;
        case lastfm:
        case librefm:
            return audioscrobbler_api_build_request_get_token(handle, auth);
            break;
        default:
            break;
    }
    return NULL;
}

struct http_request *api_build_request_get_session(CURL *handle, const struct api_credentials *auth)
{
    switch (auth->end_point) {
        case listenbrainz:
            break;
        case lastfm:
        case librefm:
            return audioscrobbler_api_build_request_get_session(handle, auth);
            break;
        default:
            break;
    }
    return NULL;
}

struct http_request *api_build_request_now_playing(const struct scrobble *track, CURL *handle, const struct api_credentials *auth)
{
    switch (auth->end_point) {
        case listenbrainz:
            return listenbrainz_api_build_request_now_playing(track, handle, auth);
            break;
        case lastfm:
        case librefm:
            return audioscrobbler_api_build_request_now_playing(track, handle, auth);
            break;
        default:
            break;
    }
    return NULL;
}

struct http_request *api_build_request_scrobble(const struct scrobble *tracks[], size_t track_count, CURL *handle, const struct api_credentials *auth)
{
    switch (auth->end_point) {
        case listenbrainz:
            return listenbrainz_api_build_request_scrobble(tracks, track_count, handle, auth);
            break;
        case lastfm:
        case librefm:
            return audioscrobbler_api_build_request_scrobble(tracks, track_count, handle, auth);
            break;
        default:
            break;
    }
    return NULL;
}

void http_response_free(struct http_response *res)
{
    if (NULL == res) { return; }

    if (NULL != res->body) {
        free(res->body);
        res->body = NULL;
    }

    free(res);
}

#if 0
void http_response_print(struct http_response *res)
{
}
#endif

struct http_response *http_response_new(void)
{
    struct http_response *res = malloc(sizeof(struct http_response));

    res->body = get_zero_string(MAX_BODY_SIZE);
    res->code = -1;
    res->body_length = 0;
    res->headers[0] = NULL;
    res->headers_count = 0;

    return res;
}

#if 0
struct http_request *api_build_request(message_type type, void *data)
{
    if (type == api_call_now_playing) {
        const struct scrobble *track = (struct scrobble *)data;
        return api_build_request_now_playing(track);
    }
    if (type == api_call_scrobble) {
        const struct scrobble *tracks = (struct scrobble *)data[];
        return api_build_request_scrobble(tracks);
    }
    return NULL;
}

void http_header_load_name(const char* data, char *name, size_t length)
{
    char *scol_pos = strchr(data, ':');
    //_trace("mem::loading_header_name[%p:%p:%p]", name, *name, scol_pos, data);

    if (NULL == scol_pos) { return; }

    strncpy(name, data, scol_pos - data);
}

void http_header_load_value(const char* data, char *value, size_t length)
{
    char *scol_pos = strchr(data, ':');

    if (NULL == scol_pos) { return; }

    strncpy(value, data, data + length - scol_pos);
    _trace("mem::loading_header_name[%p:%p:%lu] %s", value, scol_pos, length, value);
}

static size_t http_response_write_headers(char *buffer, size_t size, size_t nitems, void* data)
{
    if (NULL == buffer) { return 0; }
    if (NULL == data) { return 0; }
    if (0 == size) { return 0; }
    if (0 == nitems) { return 0; }

    struct http_response *res = (struct http_response*)data;
    res->headers_count = nitems;

//    fprintf(stdout, "HEADER[%lu:%lu]: %s\n", size, nitems, buffer);
//  fprintf(stdout, "[%p]\n", data);

    size_t new_size = size * nitems;

    struct http_header *h = http_header_new();
    char *scol_pos = strchr(buffer, ':');
    _trace("mem::position[%p:%lu]", scol_pos, (ptrdiff_t)(scol_pos - buffer));
    http_header_load_name(buffer, h->name, new_size);
    if (NULL != h->name) { return 0; }

    http_header_load_value(buffer, h->value, new_size);
    if (NULL != h->value) { return 0; }

    _error("header name: %s", h->name);
    _error("header value: %s", h->value);

    res->headers[res->headers_count] = h;
    res->headers_count++;

    return new_size;
}
#endif

size_t http_response_write_body(void *buffer, size_t size, size_t nmemb, void* data)
{
    if (NULL == buffer) { return 0; }
    if (NULL == data) { return 0; }
    if (0 == size) { return 0; }
    if (0 == nmemb) { return 0; }

    struct http_response *res = (struct http_response*)data;

    size_t new_size = size * nmemb;

    strncat(res->body, buffer, new_size);
    res->body_length += new_size;

    return new_size;
}

enum api_return_status curl_request(CURL *handle, const struct http_request *req, const http_request_type t, struct http_response *res)
{
    enum api_return_status ok = status_failed;
    if (t == http_post) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, req->body);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, (long)req->body_length);
    }

    char *url = http_request_get_url(req);
    if (NULL == req->body) {
        _trace("curl::request[%s]: %s", (t == http_get ? "G" : "P"), url);
    } else {
        _trace("curl::request[%s]: %s\ncurl::body[%lu]: %s", (t == http_get ? "G" : "P"), url, req->body_length, req->body);
    }

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0L);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, http_response_write_body);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, res);
#if 0
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, http_response_write_headers);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, res);
#endif

    CURLcode cres = curl_easy_perform(handle);
    /* Check for errors */
    if(cres != CURLE_OK) {
        _error("curl::error: %s", curl_easy_strerror(cres));
        goto _exit;
    }
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &res->code);
    _trace("curl::response(%p:%lu): %s", res, res->body_length, res->body);

    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &res->code);
#if 0
    if (res->code < 500) { http_response_print(res); }
#endif
    if (res->code == 200) { ok = true; }
_exit:
    free(url);
    return ok;
}

enum api_return_status api_get_request(CURL *handle, const struct http_request *req, struct http_response *res)
{
    return curl_request(handle, req, http_get, res);
}

enum api_return_status api_post_request(CURL *handle, const struct http_request *req, struct http_response *res)
{
    return curl_request(handle, req, http_post, res);
}

bool json_document_is_error(const char *buffer, const size_t length, enum api_type type)
{
    switch (type) {
        case lastfm:
        case librefm:
            return audioscrobbler_json_document_is_error(buffer, length);
            break;
        case listenbrainz:
            return listenbrainz_json_document_is_error(buffer, length);
            break;
        case unknown:
        default:
            break;
    }
    return false;
}

#endif // MPRIS_SCROBBLER_API_H
