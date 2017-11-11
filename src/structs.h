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
#include <expat.h>

#define ARG_PID             "-p"
#define ARG_HELP            "-h"
#define ARG_QUIET           "-q"
#define ARG_VERBOSE1        "-v"
#define ARG_VERBOSE2        "-vv"
#define ARG_VERBOSE3        "-vvv"

#define ARG_LASTFM          "lastfm"
#define ARG_LIBREFM         "librefm"
#define ARG_LISTENBRAINZ    "listenbrainz"

#define ARG_COMMAND_TOKEN       "token"
#define ARG_COMMAND_SESSION     "session"

#define HELP_OPTIONS        "Options:\n"\
"\t" ARG_HELP "\t\tDisplay this help\n" \
"\t" ARG_QUIET "\t\tDo not output debugging messages\n" \
"\t" ARG_VERBOSE1 "\t\tDisplay info messages\n" \
"\t" ARG_VERBOSE2 "\t\tDisplay debug messages\n" \
"\t" ARG_VERBOSE3 "\t\tDisplay tracing messages\n" \
""

#define QUEUE_MAX_LENGTH           20 // state
#define MAX_PROPERTY_LENGTH        512 //bytes


#define LASTFM_AUTH_URL            "https://www.last.fm/api/auth/?api_key=%s&token=%s"
#define LASTFM_API_BASE_URL        "ws.audioscrobbler.com"
#define LASTFM_API_VERSION         "2.0"

#define LIBREFM_AUTH_URL            "https://libre.fm/api/auth/?api_key=%s&token=%s"
#define LIBREFM_API_BASE_URL        "libre.fm"
#define LIBREFM_API_VERSION         "2.0"

#define LISTENBRAINZ_AUTH_URL       "https://listenbrainz.org/api/auth/?api_key=%s&token=%s"
#define LISTENBRAINZ_API_BASE_URL   "listenbrainz.org"
#define LISTENBRAINZ_API_VERSION    "2.0"

#define MAX_API_COUNT               3

#define MAX_NOW_PLAYING_EVENTS      10

enum api_type {
    unknown = 0,
    lastfm,
    librefm,
    listenbrainz,
};

struct api_credentials {
    bool enabled;
    bool authenticated;
    char* token;
    char* session_key;
    char* user_name;
    char* password;
    enum api_type end_point;
};

struct env_variables {
    const char* user_name;
    const char* home;
    const char* xdg_config_home;
    const char* xdg_data_home;
    const char* xdg_cache_home;
    const char* xdg_runtime_dir;
};

struct configuration {
    const char *name;
#if 0
    const char* config_file;
    const char* credentials_file;
#endif
    struct env_variables *env;
    struct api_credentials *credentials[MAX_API_COUNT];
    size_t credentials_length;
};

struct mpris_metadata {
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
};

struct mpris_properties {
    struct mpris_metadata *metadata;
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
};

struct events {
    struct event_base *base;
    struct event *sigint;
    struct event *sigterm;
    struct event *sighup;
    struct event *dispatch;
    struct event *scrobble;
    struct event *ping;
    size_t now_playing_count;
    struct event *now_playing[MAX_NOW_PLAYING_EVENTS];
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

struct dbus {
    DBusConnection *conn;
    DBusWatch *watch;
    DBusTimeout *timeout;
};

//typedef union queue {
//} queue;

struct mpris_player {
    char *mpris_name;
    struct mpris_properties *properties;
    struct mpris_properties *current;
    struct mpris_properties *queue[QUEUE_MAX_LENGTH];
    size_t queue_length;
    struct mpris_event *changed;
};

struct state {
    struct scrobbler *scrobbler;
    struct dbus *dbus;
    struct mpris_player *player;
    struct events *events;

};

enum log_levels
{
    none    = 0,
    error   = (1 << 0),
    warning = (1 << 1),
    info    = (1 << 2),
    debug   = (1 << 3),
    tracing = (1 << 4),
};

enum binary_type {
    daemon_bin,
    signon_bin,
};

struct parsed_arguments {
    char *name;
    char *pid_path;
    bool has_pid;
    bool has_help;
    bool get_token;
    bool get_session;
    enum binary_type binary;
    enum log_levels log_level;
    enum api_type service;
};

struct sighandler_payload {
    struct event_base *event_base;
    struct configuration *config;
};

#endif // MPRIS_SCROBBLER_STRUCTS_H
