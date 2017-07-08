/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef SCURL_H
#define SCURL_H

#include <curl/curl.h>

typedef struct api_request{
    char* url;
    char* body;
//    char** headers;
} api_request;

void api_get_request(api_request *req, api_response *res)
{
}
void api_post_request(api_request *req, api_response *res)
{
}

static void api_request(api_request *req, api_response *res, request_type *t)
{
    if (t == http_post) {
        curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDS, req->body);
    }
    curl_easy_setopt(easyhandle, CURLOPT_URL, req->url);

    curl_easy_perform(easyhandle);
}
#endif // SCURL_H
