/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_API_H
#define MPRIS_SCROBBLER_API_H

#include "md5.h"

#include <inttypes.h>
#include <json-c/json.h>
#include <stdbool.h>

#define MIN_TRACK_LENGTH                30.0L // seconds
#define NOW_PLAYING_DELAY               65.0L //seconds

#define CONTENT_TYPE_XML            "application/xml"
#define CONTENT_TYPE_JSON           "application/json"

#define API_HEADER_AUTHORIZATION_NAME "Authorization"
#define API_HEADER_AUTHORIZATION_VALUE_TOKENIZED "Token %s"

#define VALUE_SEPARATOR ", "

enum api_return_status {
    status_failed  = 0, // failed == bool false
    status_ok           // ok == bool true
};

struct api_response {
    struct api_error *error;
    enum api_return_status status;
};

typedef enum message_types {
    api_call_authenticate,
    api_call_now_playing,
    api_call_scrobble,
} message_type;


#define MAX_SCHEME_LENGTH 5
#define MAX_HOST_LENGTH 512

struct api_endpoint {
    char scheme[MAX_SCHEME_LENGTH + 1];
    char host[MAX_HOST_LENGTH + 1];
    char path[FILE_PATH_MAX + 1];
};

#include "audioscrobbler_api.h"
#include "listenbrainz_api.h"

#define HTTP_HEADER_CONTENT_TYPE "Content-Type"

static char *http_response_headers_content_type(const struct http_response *res)
{
    assert(res->headers);
    const size_t headers_count = arrlen(res->headers);
    for (size_t i = 0; i < headers_count; i++) {
        struct http_header *current = res->headers[i];

        if(strncmp(current->name, HTTP_HEADER_CONTENT_TYPE, strlen(HTTP_HEADER_CONTENT_TYPE)) == 0) {
             return current->value;
        }
    }
    return NULL;
}

#if 0
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

static void api_get_url(CURLU *url, const struct api_endpoint *endpoint)
{
    if (NULL == endpoint) { return; }
    if (NULL == url) { return; }

    CURLUcode result = curl_url_set(url, CURLUPART_SCHEME, endpoint->scheme, 0);
    if (CURLE_OK != result) {
        _warn("curl::build_URL_failed: %s", curl_url_strerror(result));
        return;
    }
    result = curl_url_set(url, CURLUPART_HOST, endpoint->host, 0);
    if (CURLE_OK != result) {
        _warn("curl::build_URL_failed: %s", curl_url_strerror(result));
        return;
    }
    result = curl_url_set(url, CURLUPART_PATH, endpoint->path, 0);
    if (CURLE_OK != result) {
        _warn("curl::build_URL_failed: %s", curl_url_strerror(result));
    }
}

static void api_endpoint_free(struct api_endpoint *api)
{
    free(api);
}

static size_t endpoint_get_scheme(char *result, const char *custom_url)
{
    if (NULL == result) { return 0; }

    const char *scheme = "https";
    if (NULL != custom_url && strncmp(custom_url, "http://", 7) == 0) {
        scheme = "http";
    }

    const size_t scheme_len = strlen(scheme);
    memcpy(result, scheme, min(scheme_len, MAX_SCHEME_LENGTH));

    return scheme_len;
}

static size_t endpoint_get_host(char *result, const enum api_type type, const enum end_point_type endpoint_type, const char *custom_url)
{
    if (NULL == result) { return 0; }

    const char* host = NULL;
    size_t host_len = 0;
    if (NULL != custom_url && strlen(custom_url) != 0) {
        bool url_has_scheme = false;
        size_t url_start = 0;
        if (strncmp(custom_url, "https://", 8) == 0) {
            url_has_scheme = true;
            url_start = 8;
        } else if (strncmp(custom_url, "http://", 7) == 0) {
            url_has_scheme = true;
            url_start = 7;
        }
        if (url_has_scheme) {
            host = (custom_url + url_start);
        } else {
            host = custom_url;
        }
        const char *base_path = strchr(host, '/');
        if (NULL == base_path) {
            host_len = strlen(host);
        } else {
            host_len = base_path - host;
        }
    } else {
        switch (type) {
            case api_lastfm:
                switch (endpoint_type) {
                    case authorization_endpoint:
                        host = LASTFM_AUTH_URL;
                        host_len = 11;
                        break;
                    case scrobble_endpoint:
                        host = LASTFM_API_BASE_URL;
                        host_len = 21;
                        break;
                    case unknown_endpoint:
                    default:
                        host = "";
                }
                break;
            case api_librefm:
                switch (endpoint_type) {
                    case authorization_endpoint:
                        host = LIBREFM_AUTH_URL;
                        host_len = 8;
                        break;
                    case scrobble_endpoint:
                        host = LIBREFM_API_BASE_URL;
                        host_len = 8;
                        break;
                    case unknown_endpoint:
                    default:
                        host = "";
                }
                break;
            case api_listenbrainz:
                switch (endpoint_type) {
                    case authorization_endpoint:
                        host = LISTENBRAINZ_AUTH_URL;
                        host_len = 34;
                        break;
                    case scrobble_endpoint:
                        host = LISTENBRAINZ_API_BASE_URL;
                        host_len = 20;
                        break;
                    case unknown_endpoint:
                    default:
                        host = "";
                }
                break;
            case api_unknown:
            default:
                return 0;
        }
    }

    memcpy(result, host, min(host_len, MAX_HOST_LENGTH));
    return host_len;
}

static size_t endpoint_get_base_path(char *result, const char *custom_url)
{
    if (NULL == result) { return 0; }

    size_t url_start = 0;
    if (strncmp(custom_url, "https://", 8) == 0) {
        url_start = 8;
    } else if (strncmp(custom_url, "http://", 7) == 0) {
        url_start = 7;
    }

    const char* base_path = strchr(custom_url + url_start, '/');
    if (NULL == base_path) {
        result[0] = '\0';
        return 0;
    }
    const size_t path_len = strlen(base_path);
    memcpy(result, base_path, min(path_len, FILE_PATH_MAX));
    return path_len;
}

static size_t endpoint_get_path(char *result, const enum api_type type, const enum end_point_type endpoint_type, const char * custom_url)
{
    if (NULL == result) { return 0; }
    const char *path = NULL;

    size_t path_len = 0;
    switch (type) {
        case api_lastfm:
            switch (endpoint_type) {
                case authorization_endpoint:
                    path = "/" LASTFM_AUTH_PATH;
                    path_len = 10;
                    break;
                case scrobble_endpoint:
                    path = "/" LASTFM_API_VERSION "/";
                    path_len = 5;
                    break;
                case unknown_endpoint:
                default:
                    path = "";
            }
            break;
        case api_librefm:
            switch (endpoint_type) {
                case authorization_endpoint:
                    path = "/" LIBREFM_AUTH_PATH;
                    path_len = 10;
                    break;
                case scrobble_endpoint:
                    path = "/" LIBREFM_API_VERSION "/";
                    path_len = 5;
                    break;
                case unknown_endpoint:
                default:
                    path = "";
            }
            break;
        case api_listenbrainz:
            switch (endpoint_type) {
                case authorization_endpoint:
                    path = "/" LISTENBRAINZ_API_VERSION "/";
                    path_len = 3;
                    break;
                case scrobble_endpoint:
                    path = "/" LISTENBRAINZ_API_VERSION "/" API_ENDPOINT_SUBMIT_LISTEN;
                    path_len = 17;
                    break;
                case unknown_endpoint:
                default:
                    path = "";
            }
            break;
        case api_unknown:
        default:
            path = NULL;
            break;
    }
    if (NULL == path) {
        return 0;
    }
    char full_path[FILE_PATH_MAX + 1];
    path_len += endpoint_get_base_path(full_path, custom_url);
    strcat(full_path, path);

    memcpy(result, full_path, min(path_len, FILE_PATH_MAX) + 1);
    return path_len;
}

static struct api_endpoint *endpoint_new(const struct api_credentials *creds, const enum end_point_type api_endpoint)
{
    if (NULL == creds) { return NULL; }

    struct api_endpoint *result = calloc(1, sizeof(struct api_endpoint));

    const enum api_type type = creds->end_point;
    endpoint_get_scheme(result->scheme, creds->url);
    endpoint_get_host(result->host, type, api_endpoint, creds->url);
    endpoint_get_path(result->path, type, api_endpoint, creds->url);

    return result;
}

static struct api_endpoint *auth_endpoint_new(const struct api_credentials *creds)
{
    return endpoint_new(creds, authorization_endpoint);
}

struct api_endpoint *api_endpoint_new(const struct api_credentials *creds)
{
    return endpoint_new(creds, scrobble_endpoint);
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

static void http_header_free(struct http_header *header)
{
    if (NULL == header) { return; }

    free(header);
}

static void http_headers_free(struct http_header **headers)
{
    if (NULL == headers) { return; }
    if (NULL == *headers) { return; }
    const size_t headers_count = arrlen(headers);
    if (headers_count > 0) {
        for (int i = (int)headers_count - 1; i >= 0 ; i--) {
            if (NULL != headers[i]) {
                struct http_header *header = arrpop(headers);
                http_header_free(header);
            }
        }
        assert(arrlen(headers) == 0);
        arrfree(headers);
    }
}

static void http_request_clean(struct http_request *req)
{
    if (NULL == req) { return; }
    if (NULL != req->url) { curl_url_cleanup(req->url); }

    api_endpoint_free(req->end_point);
    http_headers_free(req->headers);
}

static void http_request_init(struct http_request *req)
{
    req->url         = curl_url();
    memset(&req->body, 0x0, MAX_BODY_SIZE);
    req->body_length = 0;
    req->end_point   = NULL;
    req->headers     = NULL;
    time(&req->time);
}

static void print_http_request(const struct http_request *req)
{
    char *url;
    curl_url_get(req->url, CURLUPART_URL, &url, CURLU_PUNYCODE|CURLU_GET_EMPTY);
    _trace("http::req[%p]%s: %s", req, (req->request_type == http_get ? "GET" : "POST"), url);
    curl_free(url);

    const size_t headers_count = arrlen(req->headers);
    if (headers_count > 0) {
        _trace("http::req::headers[%zd]:", headers_count);
        for (size_t i = 0; i < headers_count; i++) {
            _trace("\theader[%zd]: %s:%s", i, req->headers[i]->name, req->headers[i]->value);
        }
    }
    if (req->request_type != http_get) {
        _trace("http::req[%zu]: %s", req->body_length, req->body);
    }
}

static void print_http_response(struct http_response *resp)
{
    _trace("http::resp[%p]: %u", resp, resp->code);
    const size_t headers_count = arrlen(resp->headers);
    if (headers_count > 0) {
        _trace("http::resp::headers[%zd]:", headers_count);
        for (size_t i = 0; i < headers_count; i++) {
            _trace("\theader[%zd]: %s:%s", i, resp->headers[i]->name, resp->headers[i]->value);
        }
    }
}

static bool credentials_valid(const struct api_credentials *c)
{
    return listenbrainz_valid_credentials(c) || audioscrobbler_valid_credentials(c);
}

static const char *api_get_application_secret(const enum api_type type)
{
    switch (type) {
#ifdef LIBREFM_API_SECRET
        case api_librefm:
            return LIBREFM_API_SECRET;
            break;
#endif
#ifdef LASTFM_API_SECRET
        case api_lastfm:
            return LASTFM_API_SECRET;
            break;
#endif
#ifdef LISTENBRAINZ_API_SECRET
        case api_listenbrainz:
            return LISTENBRAINZ_API_SECRET;
#endif
            break;
        case api_unknown:
        default:
            return NULL;
    }
}

static const char *api_get_application_key(const enum api_type type)
{
    switch (type) {
#ifdef LIBREFM_API_KEY
        case api_librefm:
            return LIBREFM_API_KEY;
            break;
#endif
#ifdef LASTFM_API_KEY
        case api_lastfm:
            return LASTFM_API_KEY;
            break;
#endif
#ifdef LISTENBRAINZ_API_KEY
        case api_listenbrainz:
            return LISTENBRAINZ_API_KEY;
            break;
#endif
        case api_unknown:
        default:
            return NULL;
    }
}

static void api_get_auth_url(CURLU *auth_url, const struct api_credentials *credentials)
{
    if (NULL == credentials) { return; }
    if (NULL == auth_url) { return; }

    const enum api_type type = credentials->end_point;
    const char *token = credentials->token;
    if (NULL == token) { return; }

    struct api_endpoint *auth_endpoint = auth_endpoint_new(credentials);

    switch(type) {
        case api_lastfm:
        case api_librefm:
           api_get_url(auth_url, auth_endpoint);
        break;
        case api_listenbrainz:
        case api_unknown:
        default:
           break;
    }
    const char *api_key = api_get_application_key(type);
    append_api_key_query_param(auth_url, api_key, NULL, NULL);
    append_token_query_param(auth_url, token, NULL);

    api_endpoint_free(auth_endpoint);
}

static void api_build_request_get_token(struct http_request *req, const struct api_credentials *auth, CURL *handle)
{
    switch (auth->end_point) {
        case api_listenbrainz:
            break;
        case api_lastfm:
        case api_librefm:
            audioscrobbler_api_build_request_get_token(req, auth, handle);
            break;
        case api_unknown:
        default:
            break;
    }
}

static void api_build_request_get_session(struct http_request *req, const struct api_credentials *auth, CURL *handle)
{
    switch (auth->end_point) {
        case api_listenbrainz:
            break;
        case api_lastfm:
        case api_librefm:
            audioscrobbler_api_build_request_get_session(req, auth, handle);
            break;
        case api_unknown:
        default:
            break;
    }
}

static void api_build_request_now_playing(struct http_request *req, const struct scrobble *tracks[], const unsigned track_count,
    const struct api_credentials *auth, CURL *handle)
{
    switch (auth->end_point) {
        case api_listenbrainz:
            listenbrainz_api_build_request_now_playing(req, tracks, track_count, auth);
            break;
        case api_lastfm:
        case api_librefm:
            audioscrobbler_api_build_request_now_playing(req, tracks, track_count, auth, handle);
            break;
        case api_unknown:
        default:
            break;
    }
}

static void api_build_request_scrobble(struct http_request *req, const struct scrobble *tracks[MAX_QUEUE_LENGTH],
    const unsigned track_count, const struct api_credentials *auth, CURL *handle)
{
    switch (auth->end_point) {
        case api_listenbrainz:
            listenbrainz_api_build_request_scrobble(req, tracks, track_count, auth);
            break;
        case api_lastfm:
        case api_librefm:
            audioscrobbler_api_build_request_scrobble(req, tracks, track_count, auth, handle);
            break;
        case api_unknown:
        default:
            break;
    }
}

static struct http_header *http_header_new(void)
{
    struct http_header *header = calloc(1, sizeof(struct http_header));
    return header;
}

struct http_header *http_content_type_header_new (void)
{
    struct http_header *header = http_header_new();
    strncpy(header->name, HTTP_HEADER_CONTENT_TYPE, (MAX_HEADER_NAME_LENGTH - 1));
    strncpy(header->value, CONTENT_TYPE_JSON, (MAX_HEADER_VALUE_LENGTH - 1));

    return header;
}

struct http_header *http_authorization_header_new (const char *token)
{
    struct http_header *header = http_header_new();
    strncpy(header->name, API_HEADER_AUTHORIZATION_NAME, (MAX_HEADER_NAME_LENGTH - 1));
    snprintf(header->value, MAX_HEADER_VALUE_LENGTH, API_HEADER_AUTHORIZATION_VALUE_TOKENIZED, token);

    return header;
}

static void http_response_clean(struct http_response *res)
{
    if (NULL == res) { return; }

    memset(res->body, 0x0, MAX_BODY_SIZE);
    res->body_length = 0;
    if (NULL != res->headers) {
        http_headers_free(res->headers);
    }
    res->headers = NULL;
    res->code = -1;
}

static void http_request_print(const struct http_request *req, const enum log_levels log)
{
    assert(req->url);
    char *url;
    curl_url_get(req->url, CURLUPART_URL, &url, CURLU_PUNYCODE|CURLU_GET_EMPTY);
    _log(log, "  request[%s]: %s", (req->request_type == http_get ? "GET" : "POST"), url);
    curl_free(url);

    assert(req->body);
    if (req->body_length > 0) {
        _log(log, "    request::body(%zu): %s", req->body_length, req->body);
    }
    if (log != log_tracing2) { return; }

    const size_t headers_count = arrlen(req->headers);
    if (headers_count == 0 || NULL == req->headers) {
        return;
    }
    for (size_t i = 0; i < headers_count; i++) {
        struct http_header *h = req->headers[i];
        if (NULL == h) { continue; }
        _log(log, "    request::headers[%zd]: %s: %s", i, h->name, h->value);
    }
}

static void http_response_print(const struct http_response *res, const enum log_levels log)
{
    if (NULL == res) { return; }

    _log(log, "  response::status: %zd ", res->code);
    if (res->code < 100) { return; }

    if (res->body_length > 0) {
        _log(log, "    response(%p:%lu): %s", res, res->body_length, res->body);
    }
    if (log != log_tracing2) { return; }

    const size_t headers_count = arrlen(res->headers);
    if (headers_count == 0 || NULL == res->headers) {
        return;
    }

    for (size_t i = 0; i < headers_count; i++) {
        struct http_header *h = res->headers[i];
        if (NULL == h) { continue; }
        _log(log, "    response::headers[%zd]: %s: %s", i, h->name, h->value);
    }
}

static void http_response_init(struct http_response *res)
{
    memset(res->body, 0x0, sizeof(res->body));
    res->code = -1;
    res->body_length = 0;
    res->headers = NULL;
}

static struct http_response *http_response_new(void)
{
    struct http_response *res = malloc(sizeof(struct http_response));
    http_response_clean(res);
    return res;
}

static void http_header_load(const char *data, const size_t length, struct http_header *h)
{
    if (NULL == data) { return; }
    if (NULL == h) { return; }
    if (length == 0) { return; }
    const char *scol_pos = strchr(data, ':');
    if (NULL == scol_pos) { return; }

    const size_t name_length = (size_t)(scol_pos - data);

    if (name_length < 2) { return; }

    const size_t value_length = length - name_length - 1;
    strncpy(h->name, data, name_length);

    strncpy(h->value, scol_pos + 2, value_length - 2); // skip : and space
}

static bool json_document_is_error(const char *buffer, const size_t length, enum api_type type)
{
    switch (type) {
        case api_lastfm:
        case api_librefm:
            return audioscrobbler_json_document_is_error(buffer, length);
            break;
        case api_listenbrainz:
            return listenbrainz_json_document_is_error(buffer, length);
            break;
        case api_unknown:
        default:
            break;
    }
    return false;
}

static void api_response_get_token_json(const char *buffer, const size_t length, struct api_credentials *credentials)
{
    switch (credentials->end_point) {
        case api_lastfm:
        case api_librefm:
            audioscrobbler_api_response_get_token_json(buffer, length, credentials);
            break;
        case api_listenbrainz:
        case api_unknown:
        default:
            break;
    }
}

static void api_response_get_session_key_json(const char *buffer, const size_t length, struct api_credentials *credentials)
{
    switch (credentials->end_point) {
        case api_lastfm:
        case api_librefm:
            audioscrobbler_api_response_get_session_key_json(buffer, length, credentials);
            break;
        case api_listenbrainz:
        case api_unknown:
        default:
            break;
    }
}

#endif // MPRIS_SCROBBLER_API_H
