/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_API_H
#define MPRIS_SCROBBLER_API_H

#include <curl/curl.h>
#include <expat.h>
#include <inttypes.h>
#include <stdbool.h>

#define MAX_HEADERS                     10
#define MAX_XML_NODES                   20
#define MAX_XML_ATTRIBUTES              10
#define MAX_URL_LENGTH                  1024
#define MAX_BODY_SIZE                   16384
#define API_XML_ROOT_NODE_NAME          "lfm"
#define API_XML_STATUS_ATTR_NAME        "status"
#define API_XML_STATUS_VALUE_OK         "ok"
#define API_XML_STATUS_VALUE_FAILED     "failed"
#define API_XML_ERROR_NODE_NAME         "error"
#define API_XML_ERROR_CODE_ATTR_NAME    "code"

typedef enum api_return_statuses {
    ok      = 0,
    failed,
} api_return_status;

typedef enum api_return_codes {
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
} api_return_code;

typedef enum api_node_types {
    api_node_type_unknown,
    // all
    api_node_type_document,
    api_node_type_root,
    api_node_type_error,
    // track.nowPlaying
    api_node_type_now_playing,
    // track.scrobble
    api_node_type_scrobbles,
    api_node_type_scrobble,
    // auth.getSession
    api_node_type_session,
    api_node_type_session_name,
    api_node_type_session_key,
} api_node_type;

struct api_error {
    api_return_code code;
    char* message;
};

struct api_response {
    api_return_status status;
    struct api_error *error;
};

struct xml_attribute {
    char *name;
    char *value;
};

struct xml_state {
    struct xml_node *doc;
    struct xml_node *current_node;
};

struct xml_node {
    api_node_type type;
    char *name;
    struct xml_node *parent;
    size_t attributes_count;
    size_t children_count;
    size_t content_length;
    size_t name_length;
    char *content;
    struct xml_attribute *attributes[MAX_XML_ATTRIBUTES];
    struct xml_node *children[MAX_XML_NODES];
};

typedef enum message_types {
    api_call_authenticate,
    api_call_now_playing,
    api_call_scrobble,
} message_type;

struct http_response {
    unsigned short code;
    char* body;
    size_t body_length;
};

typedef enum http_request_types {
    http_get,
    http_post,
    http_put,
    http_head,
    http_patch,
} http_request_type;

struct api_endpoint {
    char* scheme;
    char *host;
    char *path;
};

struct http_request {
    http_request_type request_type;
    struct api_endpoint *end_point;
    char *query;
    char *body;
    char* headers[MAX_HEADERS];
    size_t header_count;
};

static void xml_attribute_free(struct xml_attribute *attr)
{
    if (NULL == attr) { return; }
    if (NULL != attr->name) {
        free(attr->name);
    }
    if (NULL != attr->value) {
        free(attr->value);
    }
    free(attr);
}

static struct xml_attribute *xml_attribute_new(const char *name, size_t name_length, const char *value, size_t value_length)
{
    if (NULL == name) { return NULL; }
    if (NULL == value) { return NULL; }
    if (0 == name_length) { return NULL; }
    if (0 == value_length) { return NULL; }

    struct xml_attribute *attr = calloc(1, sizeof(struct xml_attribute));

    attr->name = calloc(1, sizeof(char) * (1 + name_length));
    strncpy (attr->name, name, name_length);

    attr->value = calloc(1, sizeof(char) * (1 + value_length));
    strncpy (attr->value, value, value_length);

    return attr;
}

static void xml_node_free(struct xml_node *node)
{
    if (NULL == node) { return; }

    _trace("xml::free_node(%p//%p):children: %u, attributes: %u", node, node->content, node->children_count, node->attributes_count);

    if (NULL != node->content) {
        free(node->content);
        node->content_length = 0;
    }
    if (NULL != node->name) {
        free(node->name);
        node->name_length = 0;
    }
    for (size_t i = 0; i < node->children_count; i++) {
        if (NULL != node->children[i]) {
            xml_node_free(node->children[i]);
            node->children_count--;
        }
    }
    for (size_t j = 0; j < node->attributes_count; j++) {
        if (NULL != node->attributes[j]) {
            xml_attribute_free(node->attributes[j]);
            node->attributes_count--;
        }
    }

    free(node);
}

static struct xml_attribute *xml_node_get_attribute(struct xml_node *node, const char *key, size_t key_len)
{
    if (NULL == node) { return NULL; }
    if (node->attributes_count == 0) { return NULL; }
    for (size_t i = 0; i < node->attributes_count; i++) {
        struct xml_attribute *attr = node->attributes[i];
        if (strncmp(attr->name, key, key_len) == 0) {
            return attr;
        }
    }
    return NULL;
}

static bool xml_node_is_error(struct xml_node *node)
{
    if (NULL == node) { return false;}
    bool status = false;
    if (node->type == api_node_type_error) {
        status = true;
    }
    return status;
}

static bool xml_document_is_error(struct xml_node *node)
{
    bool status = true;
    if (NULL == node) { return status;}
    if (node->children_count == 0) { return status; }
    if (node->type != api_node_type_document) { return status; }
    if (node->children_count == 1) {
        struct xml_node *root = node->children[0];
        if (NULL == root) { return status; }
        if (root->type != api_node_type_root) { return status; }
        if (root->children_count == 0) { return status; }
        if (root->children_count == 1) {
            struct xml_node *err = root->children[0];
            if (NULL == err) { return status; }
            if (xml_node_is_error(err)) {
                int status_code = -1;
                struct xml_attribute *status = xml_node_get_attribute(err, API_XML_ERROR_CODE_ATTR_NAME, strlen(API_XML_ERROR_CODE_ATTR_NAME));
                if (NULL != status) {
                    status_code = strtol(status->value, NULL, 0);
                }
                _warn("api::response::status(%d): %s", status_code, err->content);
                return status;
            } else {
                status = false;
            }
        }
    }
    return status;
}

static struct xml_node *xml_node_new(void)
{
    struct xml_node *node = malloc(sizeof(struct xml_node));
    node->parent = NULL;
    node->content = NULL;
    node->name = NULL;
    node->type = api_node_type_unknown;

    //*node->children = calloc(MAX_XML_NODES, sizeof(struct xml_node));
    //*node->attributes = calloc(MAX_XML_ATTRIBUTES, sizeof(struct xml_attribute));
#if 0
    for (size_t ni = 0; ni < MAX_XML_NODES; ni++) {
        node->children[ni] = malloc(sizeof(struct xml_node));
    }
    for (size_t ai = 0; ai < MAX_XML_ATTRIBUTES; ai++) {
        node->attributes[ai] = malloc(sizeof(struct xml_attribute));
    }
#endif

    node->attributes_count = 0;
    node->children_count = 0;
    node->content_length = 0;
    node->name_length = 0;

    return node;
}

static struct xml_node *xml_document_new(void)
{
    const char *doc_type = "document";
    struct xml_node *doc = xml_node_new();
    doc->name = calloc(1, strlen(doc_type) + 1);
    strncpy(doc->name, doc_type, strlen(doc_type));
    doc->type = api_node_type_document;

    _trace("xml::doc_new(%p)", doc);
    return doc;
}

static void xml_document_free(struct xml_node *doc)
{
    if (NULL == doc) { return; }
    xml_node_free(doc);
}

void xml_node_print (struct xml_node *node, short unsigned level)
{
    if (NULL == node) { return; }
    const char *tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
    char *padding = calloc(1, sizeof(char) * (level + 1));
    strncpy(padding, tabs, level);

    _trace("%sxml::node<%s>(%p)::attr:%u,nodes:%u", padding, node->name, node, node->attributes_count, node->children_count);
    if (node->content_length > 0 && NULL != node->content) {
        _trace("%s\txml::node::content: \"%s\"", padding, node->content);
    }
    for (size_t ai = 0; ai < node->attributes_count; ai++) {
        struct xml_attribute *attr = node->attributes[ai];
        _trace("%s\txml::node::attribute(%p)[%u] %s = %s", padding, attr, ai, attr->name, attr->value);
    }
    for (size_t ni = 0; ni < node->children_count; ni++) {
        struct xml_node *n = node->children[ni];
        if (NULL != n) {
            xml_node_print(n, level+1);
        }
    }
    free(padding);
}

int string_trim(char **, size_t, const char *);
static void XMLCALL text_handle(void *data, const char* incoming, int length)
{
    if (length == 0) { return; }

    if (NULL == data) { return; }
    if (NULL == incoming) { return; }
    if (0 == length) { return; }

    struct xml_state *state = data;
    if (NULL == state->current_node) { return; }

    if (string_trim((char**)&incoming, length, NULL) == 0) { return; }

    struct xml_node *node = state->current_node;

    if (length > 0) {
        if (node->type == api_node_type_error) { }

        node->content_length = length;
        node->content = calloc(1, sizeof(char) * (length + 1));
        strncpy (node->content, incoming, length);
        _trace("xml::text_handle(%p//%u):%s", node, length, node->content);
    }
}

static struct xml_node *xml_node_append_child(struct xml_node *to, struct xml_node *child)
{
    if (NULL == to) { return NULL; }
    if (NULL == child) { return NULL; }
    _trace("xml::append_child(%p)::to(%p)", child, to);

    if (to->children_count >= MAX_XML_NODES) { return NULL; }

    to->children[to->children_count] = child;
    to->children_count++;

    return child;
}

static void XMLCALL begin_element(void *data, const char* element_name, const char **attributes)
{
    if (NULL == data) { return; }

    struct xml_state *state = data;
    _trace("xml::begin_element(%p): %s", state, element_name);

    struct xml_node *node = xml_node_new();
    node->name_length = strlen(element_name);
    if (node->name_length == 0) { return; }

    node->name = calloc(1, sizeof(char) * (node->name_length + 1));
    strncpy(node->name, element_name, node->name_length);
    if (strncmp(element_name, API_XML_ROOT_NODE_NAME, strlen(API_XML_ROOT_NODE_NAME)) == 0) {
        node->type = api_node_type_root;
    }
    if (strncmp(element_name, API_XML_ERROR_NODE_NAME, strlen(API_XML_ERROR_NODE_NAME)) == 0) {
        node->type = api_node_type_error;
    }

    for (int i = 0; attributes[i]; i += 2) {
        const char *name = attributes[i];
        const char *value = attributes[i+1];

        _trace("xml::element_attributes(%p) %s = %s", data, name, value);
        struct xml_attribute *attr = xml_attribute_new(name, strlen(name), value, strlen(value));
        if (NULL != attr) {
            node->attributes[node->attributes_count] = attr;
            node->attributes_count++;
        }
    }

    node->parent = state->current_node;
    //_trace("xml::current_node::is(%p):%s", state->current_node, state->current_node->name);
    if (NULL != state->current_node) {
        xml_node_append_child(state->current_node, node);
    }
    state->current_node = node;
}

static void XMLCALL end_element(void *data, const char *element_name)
{
    if (NULL == data) { return; }
    struct xml_state *state = data;
    struct xml_node *current_node = state->current_node;
    _trace("xml::end_element(%p): %s", current_node, element_name);
    if (strncmp(element_name, API_XML_ROOT_NODE_NAME, strlen(API_XML_ROOT_NODE_NAME)) == 0) {
        // lfm
    }
    if (strncmp(element_name, API_XML_ERROR_NODE_NAME, strlen(API_XML_ERROR_NODE_NAME)) == 0) {
        // error
    }
    // now that we build the node, we append it to either the document or the current node
    state->current_node = current_node->parent;
}

static void xml_state_free(struct xml_state *state)
{
    if (NULL == state) { return; }
    free(state);
}

static struct xml_state *xml_state_new(void)
{
    struct xml_state *state = malloc(sizeof(struct xml_state));
    state->doc = NULL;
    state->current_node = NULL;
    return state;
}

static void http_response_parse_xml_body(struct http_response *res, struct xml_node* document)
{
    if (NULL == res) { return;}
    if (res->code >= 500) { return; }

    struct xml_state *state = xml_state_new();
    state->doc = document;
    state->current_node = document;

    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, state);
    XML_SetElementHandler(parser, begin_element, end_element);
    XML_SetCharacterDataHandler(parser, text_handle);

    if (XML_Parse(parser, res->body, res->body_length, XML_TRUE) == XML_STATUS_ERROR) {
        _warn("xml::parse_error");
    }

    XML_ParserFree(parser);
    xml_state_free(state);
}

static void api_endpoint_free(struct api_endpoint *api)
{
    free(api);
}

static struct api_endpoint *api_endpoint_new(api_type type)
{
    struct api_endpoint *result = malloc(sizeof(struct api_endpoint));
    result->scheme = "https";
    switch (type) {
        case  lastfm:
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
    }

    return result;
}

#if 0
static const char* get_api_error_message(api_return_code code)
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
    for (size_t i = 0; i < req->header_count; i++) {
        if (NULL != req->headers[i]) { free(req->headers[i]); }
    }
    api_endpoint_free(req->end_point);
    free(req);
}

static struct http_request *http_request_new(void)
{
    struct http_request *req = malloc(sizeof(struct http_request));
    req->body = NULL;
    req->query = NULL;
    req->end_point = NULL;
    req->headers[0] = NULL;
    req->header_count = 0;

    return req;
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
struct http_request *api_build_request_now_playing(const struct scrobble *track, CURL *handle, api_type type)
{
    struct http_request *req = http_request_new();
    char* body = get_zero_string(MAX_BODY_SIZE);
    strncat(body, "artist=", 7);
    char *artist = curl_easy_escape(handle, track->artist, strlen(track->artist));
    strncat(body, artist, strlen(artist));
    strncat(body, "&", 1);
    free(artist);

    strncat(body, "track=", 6);
    char *title = curl_easy_escape(handle, track->title, strlen(track->title));
    strncat(body, title, strlen(title));
    strncat(body, "&", 1);
    free(title);

    strncat(body, "album=", 6);
    char *album = curl_easy_escape(handle, track->album, strlen(track->album));
    strncat(body, album, strlen(album));
    free(album);

    req->body = body;
    req->end_point = api_endpoint_new(type);
    return req;
}

/*
 * artist[i] (Required) : The artist name.
 * track[i] (Required) : The track name.
 * timestamp[i] (Required) : The time the track started playing, in UNIX timestamp format (integer number of seconds since 00:00:00, January 1st 1970 UTC). This must be in the UTC time zone.
 * album[i] (Optional) : The album name.
 * context[i] (Optional) : Sub-client version (not public, only enabled for certain API keys)
 * streamId[i] (Optional) : The stream id for this track received from the radio.getPlaylist service, if scrobbling Last.fm radio
 * chosenByUser[i] (Optional) : Set to 1 if the user chose this song, or 0 if the song was chosen by someone else (such as a radio station or recommendation service). Assumes 1 if not specified
 * trackNumber[i] (Optional) : The track number of the track on the album.
 * mbid[i] (Optional) : The MusicBrainz Track ID.
 * albumArtist[i] (Optional) : The album artist - if this differs from the track artist.
 * duration[i] (Optional) : The length of the track in seconds.
 * api_key (Required) : A Last.fm API key.
 * api_sig (Required) : A Last.fm method signature. See authentication for more information.
 * sk (Required) : A session key generated by authenticating a user via the authentication protocol.
 *
 */
struct http_request *api_build_request_scrobble(const struct scrobble *tracks[], size_t track_count, CURL *handle, api_type type)
{
    struct http_request *request = http_request_new();

    char* body = get_zero_string(MAX_BODY_SIZE);
    strncpy(body, "method=track.scrobble&", 22);

    for (size_t i = 0; i < track_count; i++) {
        const struct scrobble *track = tracks[i];

        strncat(body, "artist[]=", 7);
        char *artist = curl_easy_escape(handle, track->artist, strlen(track->artist));
        strncat(body, artist, strlen(artist));
        strncat(body, "&", 1);
        free(artist);

        strncat(body, "track[]=", 6);
        char *title = curl_easy_escape(handle, track->title, strlen(track->title));
        strncat(body, title, strlen(title));
        strncat(body, "&", 1);
        free(title);

        strncat(body, "album[]=", 6);
        char *album = curl_easy_escape(handle, track->album, strlen(track->album));
        strncat(body, album, strlen(album));
        free(album);
    }
    request->body = body;
    request->end_point = api_endpoint_new(type);

    return request;
}

void http_response_free(struct http_response *res)
{
    if (NULL == res) { return; }

    if (NULL != res->body) { free(res->body); }

    free(res);
}

struct http_response *http_response_new(void)
{
    struct http_response *res = malloc(sizeof(struct http_response));

    res->body = get_zero_string(MAX_BODY_SIZE);
    res->code = -1;
    res->body_length = 0;

    return res;
}

#if 0
struct http_request* api_build_request(message_type type, void* data)
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
#endif

static char *http_request_get_url(struct api_endpoint *endpoint)
{
    if (NULL == endpoint) { return NULL; }
    char* url = get_zero_string(MAX_URL_LENGTH);

    strncat(url, endpoint->scheme, strlen(endpoint->scheme));
    strncat(url, "://", 3);
    strncat(url, endpoint->host, strlen(endpoint->host));
    strncat(url, endpoint->path, strlen(endpoint->path));

    return url;
}

static size_t http_response_write_body(void *buffer, size_t size, size_t nmemb, struct http_response *res)
{
    if (NULL == buffer) { return 0; }
    if (NULL == res) { return 0; }
    if (0 == size) { return 0; }
    if (0 == nmemb) { return 0; }

    size_t new_size = size * nmemb;

    strncat(res->body, buffer, new_size);
    res->body_length += new_size;

    return new_size;
}

static bool curl_request(CURL *handle, const struct http_request *req, const http_request_type t, struct http_response *res)
{
    if (t == http_post) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, req->body);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, (long)strlen(req->body));
    }
    bool ok = false;
    char *url = http_request_get_url(req->end_point);
    _trace("curl::request: %s %s", url, req->body);
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, http_response_write_body);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, res);

    CURLcode cres = curl_easy_perform(handle);
    /* Check for errors */
    if(cres != CURLE_OK) {
        _error("curl::error: %s", curl_easy_strerror(cres));
        goto _exit;
    }
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &res->code);
    _trace("curl::response(%p:%lu): %s", res, res->body_length, res->body);

    if (res->code < 500) {
        struct xml_node *document = xml_document_new();
        http_response_parse_xml_body(res, document);

        ok = !xml_document_is_error(document);

        xml_node_print(document, 0);
        xml_document_free(document);
    }
_exit:
    free(url);
    return ok;
}

bool api_get_request(CURL *handle, const struct http_request *req, struct http_response *res)
{
    return curl_request(handle, req, http_get, res);
}

bool api_post_request(CURL *handle, const struct http_request *req, struct http_response *res)
{
    return curl_request(handle, req, http_post, res);
}

#endif // MPRIS_SCROBBLER_API_H
