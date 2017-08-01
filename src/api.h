/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef API_H
#define API_H

#define MAX_URL_LENGTH 1024
#define MAX_BODY_SIZE 16384
#define API_XML_ROOT_NODE_NAME           "lfm"
#define API_XML_STATUS_ATTR_NAME         "status"
#define API_XML_STATUS_VALUE_OK          "ok"
#define API_XML_STATUS_VALUE_FAILED      "failed"
#define API_XML_ERROR_NODE_NAME          "error"
#define API_XML_ERROR_CODE_ATTR_NAME     "code"

void xml_document_free(struct xml_document *doc)
{
    if (NULL == doc) { return; }

    free(doc);
}

struct xml_document *xml_document_new()
{
    struct xml_document *doc = malloc(sizeof(struct xml_document));
    doc->current_node = NULL;

    return doc;
}

void xml_node_free(struct xml_node *node)
{
    if (NULL == node) { return; }

    free(node);
}

struct xml_node *xml_node_new()
{
    struct xml_node *node = malloc(sizeof(struct xml_node));

    return node;
}

void XMLCALL text_handle(void *data, const char* incoming, int length)
{
    if (length == 0) { return; }

    if (NULL == data) { return; }
    if (NULL == incoming) { return; }

    struct xml_document *document = data;
    if (NULL == document->current_node) { return; }
#if 0
    _trace("xml::doc(%p)", document);
    _trace("xml::incoming:%ld: %s", length, incoming);
    if (document->current_node->type == api_node_type_error) {
        _trace("xml::inc(%lu): %s", length, incoming);
    }
#endif
#if 1
    char *text = calloc(1, length + 1);
    strncpy (text, incoming, length);
    fprintf(stdout, "%s\n", text);
    fprintf(stderr, "%p\n", data);
    free(text);
#endif
}

void XMLCALL begin_element(void *data, const char* element_name, const char **attributes)
{
    if (NULL == data) { return; }

    struct xml_document *document = data;
    _trace("xml::doc_cur_node(%p)", document->current_node);
    if (NULL == document->current_node) { return; }

    _trace("xml::doc(%p)", document);

    if (strncmp(element_name, API_XML_ROOT_NODE_NAME, strlen(API_XML_ROOT_NODE_NAME)) == 0) {
        // lfm
        struct xml_node *root = xml_node_new();
        root->type = api_node_type_root;
    }
    if (strncmp(element_name, API_XML_ERROR_NODE_NAME, strlen(API_XML_ERROR_NODE_NAME)) == 0) {
        // error
        struct xml_node *node = xml_node_new();
        node->type = api_node_type_error;
    }

#if 1
    printf("Node{%s}\n", element_name);
    fprintf(stderr, "%p\n", data);

    for (int i = 0; attributes[i]; i += 2) {
        printf ("\t %s = %s\n", attributes[i], attributes[i+1]);
    }
#endif
}

void XMLCALL end_element(void *data, const char *element_name)
{
    if (NULL == data) { return; }
    struct xml_document *document = data;
    if (strncmp(element_name, API_XML_ROOT_NODE_NAME, strlen(API_XML_ROOT_NODE_NAME)) == 0) {
        // lfm
    }
    if (strncmp(element_name, API_XML_ERROR_NODE_NAME, strlen(API_XML_ERROR_NODE_NAME)) == 0) {
        // error
    }
    document->current_node = NULL;
}

static void http_response_parse_xml_body(http_response *res)
{
    if (NULL == res) { return;}
    if (res->code >= 500) { return; }

    struct xml_document *document = xml_document_new();

    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, &document);
    XML_SetElementHandler(parser, begin_element, end_element);
    XML_SetCharacterDataHandler(parser, text_handle);

    if (XML_Parse(parser, res->body, res->body_length, XML_TRUE) == XML_STATUS_ERROR) {
        _warn("xml::parse_error");
    }

    XML_ParserFree(parser);
    xml_document_free(document);
}

static void api_endpoint_free(api_endpoint *api)
{
    free(api);
}

static api_endpoint *api_endpoint_new(api_type type)
{
    api_endpoint *result = malloc(sizeof(api_endpoint));
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

void http_request_free(http_request *req)
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

static http_request *http_request_new()
{
    http_request *req = malloc(sizeof(http_request));
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
http_request *api_build_request_now_playing(const scrobble *track, CURL *handle, api_type type)
{
    http_request *req = http_request_new();
    char* body = get_zero_string(MAX_BODY_SIZE);
    strncat(body, "artist=", 7);
    char *artist = curl_easy_escape(handle, track->artist, strlen(track->artist));
    strncat(body, artist, strlen(artist));
    strncat(body, "&", 1);

    strncat(body, "track=", 6);
    char *title = curl_easy_escape(handle, track->title, strlen(track->title));
    strncat(body, title, strlen(title));
    strncat(body, "&", 1);

    strncat(body, "album=", 6);
    char *album = curl_easy_escape(handle, track->album, strlen(track->album));
    strncat(body, album, strlen(album));

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
http_request* api_build_request_scrobble(scrobble *tracks[], size_t track_count, CURL *handle)
{
    http_request *request = http_request_new();

    char* body = get_zero_string(MAX_BODY_SIZE);
    strncpy(body, "method=track.scrobble&", 22);

    for (size_t i = 0; i < track_count; i++) {
        scrobble *track = tracks[i];

        strncat(body, "artist[]=", 7);
        char *artist = curl_easy_escape(handle, track->artist, strlen(track->artist));
        strncat(body, artist, strlen(artist));
        strncat(body, "&", 1);

        strncat(body, "track[]=", 6);
        char *title = curl_easy_escape(handle, track->title, strlen(track->title));
        strncat(body, title, strlen(title));
        strncat(body, "&", 1);

        strncat(body, "album[]=", 6);
        char *album = curl_easy_escape(handle, track->album, strlen(track->album));
        strncat(body, album, strlen(album));
    }
    request->body = body;

    return request;
}

void http_response_free(http_response *res)
{
    if (NULL == res) { return; }

    if (NULL != res->body) { free(res->body); }

    free(res);
}

http_response *http_response_new()
{
    http_response *res = malloc(sizeof(http_response));

    res->body = get_zero_string(MAX_BODY_SIZE);
    res->code = -1;
    res->body_length = 0;

    return res;
}

#if 0
http_request* api_build_request(message_type type, void* data)
{
    if (type == api_call_now_playing) {
        const scrobble *track = (scrobble *)data;
        return api_build_request_now_playing(track);
    }
    if (type == api_call_scrobble) {
        scrobble *tracks = (scrobble *)data[];
        return api_build_request_scrobble(tracks);
    }
    return NULL;
}
#endif

static char *http_request_get_url(api_endpoint *endpoint)
{
    if (NULL == endpoint) { return NULL; }
    char* url = get_zero_string(MAX_URL_LENGTH);

    strncat(url, endpoint->scheme, strlen(endpoint->scheme));
    strncat(url, "://", 3);
    strncat(url, endpoint->host, strlen(endpoint->host));
    strncat(url, endpoint->path, strlen(endpoint->path));

    return url;
}

static size_t http_response_write_body(void *buffer, size_t size, size_t nmemb, http_response *res)
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

static void curl_request(CURL *handle, const http_request *req, const http_request_type t, http_response *res)
{
    if (t == http_post) {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, req->body);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, (long)strlen(req->body));
    }
    char *url = http_request_get_url(req->end_point);
    _info("curl::request: %s %s", url, req->body);
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

    http_response_parse_xml_body(res);

_exit:
    free(url);
}

void api_get_request(CURL *handle, const http_request *req, http_response *res)
{
    curl_request(handle, req, http_get, res);
}

void api_post_request(CURL *handle, const http_request *req, http_response *res)
{
    curl_request(handle, req, http_post, res);
}

#endif // API_H
