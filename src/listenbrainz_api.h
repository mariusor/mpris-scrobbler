/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_LISTENBRAINZ_API_H
#define MPRIS_SCROBBLER_LISTENBRAINZ_API_H

#ifndef LISTENBRAINZ_API_KEY
#include "credentials_listenbrainz.h"
#endif

/*
 */
struct http_request *listenbrainz_api_build_request_now_playing(const struct scrobble *track, CURL *handle, const struct api_credentials *auth)
{
    return NULL;
}

/*
 */
struct http_request *listenbrainz_api_build_request_scrobble(const struct scrobble *tracks[], size_t track_count, CURL *handle, const struct api_credentials *auth)
{
    return NULL;
}

#endif // MPRIS_SCROBBLER_LISTENBRAINZ_API_H
