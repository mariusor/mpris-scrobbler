/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_API_H
#define MPRIS_SCROBBLER_API_H

#include "md5.h"

#include <inttypes.h>
#include <json-c/json.h>
#include <stdbool.h>

#define MAX_HEADER_LENGTH               256
#define MAX_HEADER_NAME_LENGTH          (MAX_URL_LENGTH / 2)
#define MAX_HEADER_VALUE_LENGTH         (MAX_URL_LENGTH / 2)
#define MAX_URL_LENGTH                  2048
#define MAX_BODY_SIZE                   16384

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
    long code;
    char *body;
    size_t body_length;
    struct http_header **headers;
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
    struct http_header **headers;
};

#include "audioscrobbler_api.h"
#include "listenbrainz_api.h"

#define HTTP_HEADER_CONTENT_TYPE "Content-Type"

char *http_response_headers_content_type(struct http_response *res)
{

    int headers_count = arrlen(res->headers);
    for (int i = 0; i < headers_count; i++) {
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

char *api_get_url(struct api_endpoint *endpoint)
{
    if (NULL == endpoint) { return NULL; }
    char *url = get_zero_string(MAX_URL_LENGTH);

    strncat(url, endpoint->scheme, MAX_URL_LENGTH);
    strncat(url, "://", 4);
    strncat(url, endpoint->host, MAX_URL_LENGTH);
    strncat(url, endpoint->path, MAX_URL_LENGTH);

    return url;
}

char *http_request_get_url(const struct http_request *request)
{
    if (NULL == request) { return NULL; }
    char *url = get_zero_string(MAX_URL_LENGTH);

    strncat(url, request->url, MAX_URL_LENGTH);
    if (NULL == request->query) {
        goto _return;
    }

    size_t query_len = strlen(request->query);
    if (query_len > 0) {
        strncat(url, "?", 2);
        strncat(url, request->query, query_len + 1);
    }

_return:
    return url;
}

void api_endpoint_free(struct api_endpoint *api)
{
    if (NULL != api->host) { string_free(api->host); }
    if (NULL != api->path) { string_free(api->path); }
    if (NULL != api->scheme) { string_free(api->scheme); }
    free(api);
}

char *endpoint_get_scheme(const char *custom_url)
{
    const char *scheme = "https";
    if (NULL != custom_url && strncmp(custom_url, "http://", 7) == 0) {
        scheme = "http";
    }

    char *result = get_zero_string(8);
    strncpy(result, scheme, 8);

    return result;
}

char *endpoint_get_host(const enum api_type type, const enum end_point_type endpoint_type, const char *custom_url)
{
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
        host_len = strlen(host);
    } else {
        switch (type) {
            case api_lastfm:
                switch (endpoint_type) {
                    case auth_endpoint:
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
                    case auth_endpoint:
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
                    case auth_endpoint:
                        host = LISTENBRAINZ_AUTH_URL;
                        host_len = 54;
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
                return NULL;
        }
    }
    char *result = get_zero_string(host_len);
    strncpy(result, host, host_len + 1);

    return result;
}

char *endpoint_get_path(const enum api_type type, const enum end_point_type endpoint_type)
{
    const char *path = NULL;
    char *result = NULL;
    size_t path_len = 0;
    switch (type) {
        case api_lastfm:
            switch (endpoint_type) {
                case auth_endpoint:
                    path = "/" LASTFM_AUTH_PATH;
                    path_len = 31;
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
                case auth_endpoint:
                    path = "/" LIBREFM_AUTH_PATH;
                    path_len = 31;
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
                case auth_endpoint:
                    path = "/" LISTENBRAINZ_API_VERSION "/";
                    path_len = 3;
                    break;
                case scrobble_endpoint:
                    path = "/" LISTENBRAINZ_API_VERSION "/";
                    path_len = 3;
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
    if (NULL != path) {
        result = get_zero_string(path_len);
        strncpy(result, path, path_len + 1);
    }

    return result;
}

struct api_endpoint *endpoint_new(const struct api_credentials *creds, const enum end_point_type api_endpoint)
{
    if (NULL == creds) { return NULL; }

    struct api_endpoint *result = malloc(sizeof(struct api_endpoint));

    enum api_type type = creds->end_point;
    result->scheme = endpoint_get_scheme(creds->url);
    result->host = endpoint_get_host(type, api_endpoint, creds->url);
    result->path = endpoint_get_path(type, api_endpoint);

    return result;
}

struct api_endpoint *auth_endpoint_new(const struct api_credentials *creds)
{
    return endpoint_new(creds, auth_endpoint);
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

    if (NULL != header->name) { string_free(header->name); }
    if (NULL != header->value) { string_free(header->value); }

    free(header);
}

void http_headers_free(struct http_header **headers)
{
    if (NULL == headers) { return; }
    if (NULL == *headers) { return; }
    int headers_count = arrlen(headers);
    if (headers_count > 0) {
        for (int i = headers_count - 1; i >= 0 ; i--) {
            if (NULL != headers[i]) { http_header_free(headers[i]); }
            (void)arrpop(headers);
        }
        assert(arrlen(headers) == 0);
        arrfree(headers);
    }
}

void http_request_free(struct http_request *req)
{
    if (NULL == req) { return; }
    if (NULL != req->body) { string_free(req->body); }
    if (NULL != req->query) { string_free(req->query); }
    if (NULL != req->url) { string_free(req->url); }

    api_endpoint_free(req->end_point);
    http_headers_free(req->headers);

    free(req);
}

struct http_request *http_request_new(void)
{
    struct http_request *req = malloc(sizeof(struct http_request));
    req->url         = NULL;
    req->body        = NULL;
    req->body_length = 0;
    req->query       = NULL;
    req->end_point   = NULL;
    req->headers     = NULL;

    return req;
}

void print_http_request(const struct http_request *req)
{
    char *url = http_request_get_url(req);
    _trace("http::req[%p]%s: %s", req, (req->request_type == http_get ? "GET" : "POST"), url);
    int headers_count = arrlen(req->headers);
    if (headers_count > 0) {
        _trace("http::req::headers[%zd]:", headers_count);
        for (int i = 0; i < headers_count; i++) {
            _trace("\theader[%zd]: %s:%s", i, req->headers[i]->name, req->headers[i]->value);
        }
    }
    if (req->request_type != http_get) {
        _trace("http::req[%zu]: %s", req->body_length, req->body);
    }
    string_free(url);
}

void print_http_response(struct http_response *resp)
{
    _trace("http::resp[%p]: %u", resp, resp->code);
    int headers_count = arrlen(resp->headers);
    if (headers_count > 0) {
        _trace("http::resp::headers[%zd]:", headers_count);
        for (int i = 0; i < headers_count; i++) {
            _trace("\theader[%zd]: %s:%s", i, resp->headers[i]->name, resp->headers[i]->value);
        }
    }
}

#define MD5_DIGEST_LENGTH 16
char *api_get_signature(const char *string, const char *secret)
{
    if (NULL == string) { return NULL; }
    if (NULL == secret) { return NULL; }
    size_t string_len = strlen(string);
    size_t secret_len = strlen(secret);
    size_t len = string_len + secret_len;
    char *sig = get_zero_string(len);

    // TODO(marius): this needs to change to memcpy or strncpy
    strncat(sig, string, MAX_PROPERTY_LENGTH);
    strncat(sig, secret, MAX_PROPERTY_LENGTH);

    unsigned char sig_hash[MD5_DIGEST_LENGTH];

    md5((uint8_t*)sig, len, sig_hash);

    char *result = get_zero_string(MD5_DIGEST_LENGTH * 2 + 2);
    for (size_t n = 0; n < MD5_DIGEST_LENGTH; n++) {
        snprintf(result + 2 * n, 3, "%02x", sig_hash[n]);
    }

    string_free(sig);
    return result;
}

bool credentials_valid(struct api_credentials *c)
{
    return (NULL != c && c->enabled && (NULL != c->session_key));
}

const char *api_get_application_secret(enum api_type type)
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
    return NULL;
}

const char *api_get_application_key(enum api_type type)
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
    return NULL;
}

char *api_get_auth_url(struct api_credentials *credentials)
{
    if (NULL == credentials) { return NULL; }

    enum api_type type = credentials->end_point;
    const char *token = credentials->token;
    if (NULL == token) { return NULL; }

    struct api_endpoint *auth_endpoint = auth_endpoint_new(credentials);
    const char* base_url = NULL;

    switch(type) {
        case api_lastfm:
        case api_librefm:
           base_url = api_get_url(auth_endpoint);
           break;
        case api_listenbrainz:
        case api_unknown:
        default:
           base_url = get_zero_string(0);
           break;
    }
    const char *api_key = api_get_application_key(type);
    size_t token_len = strlen(token);
    size_t key_len = strlen(api_key);
    size_t base_url_len = strlen(base_url);

    size_t url_len = base_url_len + token_len + key_len;
    char *url = get_zero_string(url_len);

    snprintf(url, url_len, base_url, api_key, token);
    string_free((char*)base_url);
    api_endpoint_free(auth_endpoint);

    return url;
}

struct http_request *api_build_request_get_token(const struct api_credentials *auth)
{
    switch (auth->end_point) {
        case api_listenbrainz:
            break;
        case api_lastfm:
        case api_librefm:
            return audioscrobbler_api_build_request_get_token(auth);
            break;
        case api_unknown:
        default:
            break;
    }
    return NULL;
}

struct http_request *api_build_request_get_session(const struct api_credentials *auth)
{
    switch (auth->end_point) {
        case api_listenbrainz:
            break;
        case api_lastfm:
        case api_librefm:
            return audioscrobbler_api_build_request_get_session(auth);
            break;
        case api_unknown:
        default:
            break;
    }
    return NULL;
}

struct http_request *api_build_request_now_playing(const struct scrobble *tracks[], const int track_count, const struct api_credentials *auth)
{
    switch (auth->end_point) {
        case api_listenbrainz:
            return listenbrainz_api_build_request_now_playing(tracks, track_count, auth);
            break;
        case api_lastfm:
        case api_librefm:
            return audioscrobbler_api_build_request_now_playing(tracks, track_count, auth);
            break;
        case api_unknown:
        default:
            break;
    }
    return NULL;
}

struct http_request *api_build_request_scrobble(const struct scrobble *tracks[], const int track_count, const struct api_credentials *auth)
{
    switch (auth->end_point) {
        case api_listenbrainz:
            return listenbrainz_api_build_request_scrobble(tracks, track_count, auth);
            break;
        case api_lastfm:
        case api_librefm:
            return audioscrobbler_api_build_request_scrobble(tracks, track_count, auth);
            break;
        case api_unknown:
        default:
            break;
    }
    return NULL;
}

static struct http_header *http_header_new(void)
{
    struct http_header *header = malloc(sizeof(struct http_header));
    header->name = get_zero_string(MAX_HEADER_NAME_LENGTH);
    header->value = get_zero_string(MAX_HEADER_VALUE_LENGTH);

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
    strncpy(header->name, API_HEADER_AUTHORIZATION_NAME, (MAX_HEADER_VALUE_LENGTH - 1));
    snprintf(header->value, (MAX_HEADER_VALUE_LENGTH - 1), API_HEADER_AUTHORIZATION_VALUE_TOKENIZED, token);

    return header;
}

void http_response_free(struct http_response *res)
{
    if (NULL == res) { return; }

    if (NULL != res->body) {
        string_free(res->body);
        res->body = NULL;
        res->body_length = 0;
    }
    http_headers_free(res->headers);

    free(res);
}

void http_request_print(const struct http_request *req, enum log_levels log)
{
    if (NULL == req) { return; }

    char *url = http_request_get_url(req);
    _log(log, "  request[%s]: %s", (req->request_type == http_get ? "GET" : "POST"), url);
    string_free(url);
    if (req->body_length > 0 && NULL != req->body) {
        _log(log, "    request::body(%p:%zu): %s", req, req->body_length, req->body);
    }
    if (log != log_tracing2) { return; }

    int headers_count = arrlen(req->headers);
    if (headers_count > 0) {
        for (int i = 0; i < headers_count; i++) {
            struct http_header *h = req->headers[i];
            if (NULL == h) { continue; }
            _log(log, "    request::headers[%zd]: %s: %s", i, h->name, h->value);
        }
    }
}

void http_response_print(const struct http_response *res, enum log_levels log)
{
    if (NULL == res) { return; }

    _log(log, "  response(%p)::status: %zd", res, res->code);
    if (res->body_length > 0 && NULL != res->body) {
        _log(log, "    response(%p:%lu): %s", res, res->body_length, res->body);
    }
    if (log != log_tracing2) { return; }

    int headers_count = arrlen(res->headers);
    if (headers_count > 0) {
        for (int i = 0; i < headers_count; i++) {
            struct http_header *h = res->headers[i];
            if (NULL == h) { continue; }
            _log(log, "    response::headers[%zd]: %s: %s", i, h->name, h->value);
        }
    }
}

struct http_response *http_response_new(void)
{
    struct http_response *res = malloc(sizeof(struct http_response));

    res->body = get_zero_string(MAX_BODY_SIZE);
    res->code = -1;
    res->body_length = 0;
    res->headers = NULL;

    return res;
}

void http_header_load(const char *data, size_t length, struct http_header *h)
{
    if (NULL == data) { return; }
    if (NULL == h) { return; }
    if (length == 0) { return; }
    char *scol_pos = strchr(data, ':');
    if (NULL == scol_pos) { return; }

    size_t name_length = (size_t)(scol_pos - data);

    if (name_length < 2) { return; }

    size_t value_length = length - name_length - 1;
    strncpy(h->name, data, name_length);

    strncpy(h->value, scol_pos + 2, value_length - 2); // skip : and space
}

static size_t http_response_write_headers(char *buffer, size_t size, size_t nitems, void* data)
{
    if (NULL == buffer) { return 0; }
    if (NULL == data) { return 0; }
    if (0 == size) { return 0; }
    if (0 == nitems) { return 0; }

    struct http_response *res = data;

    size_t new_size = size * nitems;

    struct http_header *h = http_header_new();

    http_header_load(buffer, nitems, h);
    if (NULL == h->name  || strlen(h->name) == 0) { goto _err_exit; }

    arrput(res->headers, h);
    return new_size;

_err_exit:
    http_header_free(h);
    return new_size;
}

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

    assert (res->body_length < MAX_BODY_SIZE);
    memset(res->body + res->body_length, 0x0, MAX_BODY_SIZE - res->body_length);

    return new_size;
}

#if 0
static int curl_connection_progress(void *data, double dltotal, double dlnow, double ult, double uln)
{
    struct scrobbler_connection *conn = (struct scrobbler_connection *)data;
    _trace2("curl::progress: %s (%g/%g/%g/%g)", conn->request->url, dlnow, dltotal, ult, uln);
    return 0;
}
#endif

void build_curl_request(struct scrobbler_connection *conn)
{
    assert (NULL != conn);

    CURL *handle = conn->handle;
    const struct http_request *req = conn->request;
    struct http_response *resp = conn->response;
    struct curl_slist ***req_headers = &conn->headers;

    if (NULL == handle || NULL == req || NULL == resp) { return; }
    enum http_request_types t = req->request_type;

    if (t == http_post) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, req->body);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, (long)req->body_length);
    }

    char *url = http_request_get_url(req);
    http_request_print(req, log_tracing2);

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0L);
    int headers_count = arrlen(req->headers);
    if (headers_count > 0) {
        struct curl_slist *headers = NULL;

        for (int i = 0; i < headers_count; i++) {
            struct http_header *header = req->headers[i];
            char *full_header = get_zero_string(MAX_URL_LENGTH);
            snprintf(full_header, MAX_URL_LENGTH, "%s: %s", header->name, header->value);

            headers = curl_slist_append(headers, full_header);

            string_free(full_header);
        }
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
        arrput(*req_headers, headers);
    }
    string_free(url);

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, http_response_write_body);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, resp);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, http_response_write_headers);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, resp);
#if 0
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, curl_connection_progress);
    curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, curl_req);
#endif
}

bool json_document_is_error(const char *buffer, const size_t length, enum api_type type)
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

void api_response_get_token_json(const char *buffer, const size_t length, char **token, enum api_type type)
{
    switch (type) {
        case api_lastfm:
        case api_librefm:
            audioscrobbler_api_response_get_token_json(buffer, length, token);
            break;
        case api_listenbrainz:
        case api_unknown:
        default:
            break;
    }
}

void api_response_get_session_key_json(const char *buffer, const size_t length, char **session_key, char **name, enum api_type type)
{
    switch (type) {
        case api_lastfm:
        case api_librefm:
            audioscrobbler_api_response_get_session_key_json(buffer, length, session_key, name);
            break;
        case api_listenbrainz:
        case api_unknown:
        default:
            break;
    }
}

#endif // MPRIS_SCROBBLER_API_H
