/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef SAPI_H
#define SAPI_H

char* api_build_request_now_playing(scrobble *s)
{
    char* result = NULL;

    return result;
}

char* api_build_request_scrobble(scrobble *s)
{
    char* result = NULL;

    return result;
}

char* api_build_request(message_type type, void* data)
{
    if (type == api_call_now_playing) {
        return api_build_request_now_playing((scrobble*)data);
    }
    if (type == api_call_scrobble) {
        return api_build_request_scrobble((scrobble*)data);
    }
    return NULL;
}

void scrobbler_endpoint_destroy(api_endpoint *api)
{
    free(api);
}

api_endpoint scrobbler_endpoint_new(api_type type)
{
}

#endif // SAPI_H
