/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_STRUCTS_H
#define MPRIS_SCROBBLER_STRUCTS_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <event.h>
#include <dbus/dbus.h>
#include <curl/curl.h>
#include <expat.h>

#define QUEUE_MAX_LENGTH           20 // state
#define MAX_PROPERTY_LENGTH        512 //bytes

#define LASTFM_API_BASE_URL        "ws.audioscrobbler.com"
#define LASTFM_API_VERSION         "2.0"

#define LIBREFM_API_BASE_URL        "turtle.libre.fm"
#define LIBREFM_API_VERSION         "2.0"

#define LISTENBRAINZ_API_BASE_URL   "listenbrainz.org"
#define LISTENBRAINZ_API_VERSION    "2.0"

#define MAX_HEADERS                 10
#define MAX_NODES                   20

#define MAX_API_COUNT 10

#define MAX_NOW_PLAYING_EVENTS      10

typedef enum api_type {
    lastfm,
    librefm,
    listenbrainz,
} api_type;

struct api_credentials {
    bool enabled;
    char* user_name;
    char* password;
    api_type end_point;
};

struct configuration {
    struct api_credentials *credentials[MAX_API_COUNT];
    size_t credentials_length;
};

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

struct events {
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
        struct event *events[4];
        struct {
            struct event *dispatch;
            struct event *scrobble;
            struct event *ping;
            size_t now_playing_count;
            struct event *now_playing[MAX_NOW_PLAYING_EVENTS];
        };
    };
};

struct scrobble {
    char* title;
    char* album;
    char* artist;
    bool scrobbled;
    unsigned short track_number;
    unsigned length;
    time_t start_time;
    double play_time;
    double position;
};

typedef enum playback_states {
    undetermined = 0,
    stopped = 1 << 0,
    paused = 1 << 1,
    playing = 1 << 2
} playback_state;

struct mpris_event {
    playback_state player_state;
    bool playback_status_changed;
    bool track_changed;
    bool volume_changed;
    bool position_changed;
};

typedef struct dbus {
    DBusConnection *conn;
    DBusWatch *watch;
    DBusTimeout *timeout;
} dbus;

//typedef union queue {
//} queue;

struct mpris_player {
    char *mpris_name;
    mpris_properties *properties;
    union {
        struct {
            struct mpris_properties *current;
            struct mpris_properties *previous;
        };
        struct mpris_properties *queue[QUEUE_MAX_LENGTH];
    };
    size_t queue_length;
    struct mpris_event *changed;
};

struct state {
    struct scrobbler *scrobbler;
    dbus *dbus;
    struct mpris_player *player;
    struct events *events;

};

#endif // MPRIS_SCROBBLER_STRUCTS_H
