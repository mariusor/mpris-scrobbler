/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <event.h>
#include <dbus/dbus.h>
#include <curl/curl.h>

#define QUEUE_MAX_LENGTH           20 // state
#define MAX_PROPERTY_LENGTH        512 //bytes

#define LASTFM_API_BASE_URL "ws.audioscrobbler.com"
#define LASTFM_API_VERSION "2.0"

#define LIBREFM_API_BASE_URL "turtle.libre.fm"
#define LIBREFM_API_VERSION "2.0"

#define LISTENBRAINZ_API_BASE_URL "ws.listenbrainz.org"
#define LISTENBRAINZ_API_VERSION "2.0"

typedef enum lastfm_return_statuses {
    ok = 0,
    unavaliable = 1, //The service is temporarily unavailable, please try again.
    invalid_service = 2, //Invalid service - This service does not exist
    invalid_method = 3, //Invalid Method - No method with that name in this package
    authentication_failed = 4, //Authentication Failed - You do not have permissions to access the service
    invalid_format = 5, //Invalid format - This service doesn't exist in that format
    invalid_parameters = 6, //Invalid parameters - Your request is missing a required parameter
    invalid_resource = 7, //Invalid resource specified
    operation_failed = 8, //Operation failed - Something else went wrong
    invalid_session_key = 9, //Invalid session key - Please re-authenticate
    invalid_apy_key = 10, //Invalid API key - You must be granted a valid key by last.fm
    service_offline = 11, //Service Offline - This service is temporarily offline. Try again later.
    invalid_signature = 13, //Invalid method signature supplied
    temporary_error = 16, //There was a temporary error processing your request. Please try again
    suspended_api_key = 26, //Suspended API key - Access for your account has been suspended, please contact Last.fm
    rate_limit_exceeded = 29 //Rate limit exceeded - Your IP has made too many requests in a short period
} lastfm_call_status;

typedef enum api_type {
    lastfm = (0x01 << 1),
    librefm = (0x01 << 2),
    listenbrainz = (0x01 << 3)
} api_type;

typedef struct _credentials {
    char* user_name;
    char* password;
    api_type end_point;
} api_credentials;

#define MAX_API_COUNT 10

typedef struct global_config {
    api_credentials *credentials[MAX_API_COUNT];
    size_t credentials_length;
} configuration;

typedef struct scrobbler {
    CURL *curl;
    api_credentials *credentials[MAX_API_COUNT];
    size_t credentials_length;
} scrobbler;

typedef enum log_levels
{
    none = (1 << 0),
    tracing = (1 << 1),
    debug   = (1 << 2),
    info    = (1 << 3),
    warning = (1 << 4),
    error   = (1 << 5)
} log_level;

typedef struct mpris_metadata {
    char* album_artist;
    char* composer;
    char* genre;
    char* artist;
    char* comment;
    char* track_id;
    char* album;
    char* content_created;
    char* title;
    char* url;
    char* art_url; //mpris specific
    uint64_t length; // mpris specific
    unsigned short track_number;
    unsigned short bitrate;
    unsigned short disc_number;
} mpris_metadata;

typedef struct mpris_properties {
    mpris_metadata *metadata;
    double volume;
    int64_t position;
    char* player_name;
    char* loop_status;
    char* playback_status;
    bool can_control;
    bool can_go_next;
    bool can_go_previous;
    bool can_play;
    bool can_pause;
    bool can_seek;
    bool shuffle;
} mpris_properties;

typedef struct local_events {
    struct event_base *base;
    union {
        struct event *signal_events[3];
        struct {
            struct event *sigint;
            struct event *sigterm;
            struct event *sighup;
        };
    };
    union {
        struct event *events[3];
        struct {
            struct event *now_playing;
            struct event *dispatch;
            struct event *scrobble;
        };
    };
} events;

typedef struct scrobble {
    char* title;
    char* album;
    char* artist;
    bool scrobbled;
    unsigned short track_number;
    unsigned length;
    time_t start_time;
    double play_time;
    double position;
} scrobble;

typedef enum playback_states {
    undetermined = 0,
    stopped = 1 << 0,
    paused = 1 << 1,
    playing = 1 << 2
} playback_state;

typedef struct mpris_event {
    playback_state player_state;
    bool playback_status_changed;
    bool track_changed;
    bool volume_changed;
} mpris_event;

typedef struct dbus {
    DBusConnection *conn;
    DBusWatch *watch;
    DBusTimeout *timeout;
} dbus;

typedef struct application_state {
    scrobbler *scrobbler;
    dbus *dbus;
    struct local_events *events;
    mpris_properties *properties;
    union {
        struct {
            scrobble *current;
            scrobble *previous;
        };
        scrobble *queue[QUEUE_MAX_LENGTH];
    };
    size_t queue_length;
    playback_state player_state;
} state;

#define MAX_HEADERS 10

typedef enum message_types {
    api_call_authenticate,
    api_call_now_playing,
    api_call_scrobble,
} message_type;

typedef struct api_response {
    unsigned short http_code;
} api_response;

typedef enum http_request_types {
    http_get,
    http_post,
    http_put,
    http_head,
    http_patch,
} http_request_type;

typedef struct api_endpoint {
    char* scheme;
    char *host;
    char *path;
} api_endpoint;

typedef struct api_request {
    http_request_type request_type;
    api_endpoint *end_point;
    char *query;
    char *body;
    char* headers[MAX_HEADERS];
    size_t header_count;
} api_request;

#endif // STRUCTS_H
