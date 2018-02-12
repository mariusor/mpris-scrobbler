/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_STRUCTS_H
#define MPRIS_SCROBBLER_STRUCTS_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define ARG_HELP            "-h"
#define ARG_HELP_LONG       "--help"
#define ARG_QUIET           "-q"
#define ARG_QUIET_LONG      "--quiet"
#define ARG_VERBOSE1        "-v"
#define ARG_VERBOSE2        "-vv"
#define ARG_VERBOSE3        "-vvv"
#define ARG_VERBOSE_LONG    "--verbose[=1-3]"
#define ARG_URL             "-u <example.org>"
#define ARG_URL_LONG        "--url=<example.org>"

#define ARG_LASTFM          "lastfm"
#define ARG_LIBREFM         "librefm"
#define ARG_LISTENBRAINZ    "listenbrainz"

#define ARG_COMMAND_TOKEN       "token"
#define ARG_COMMAND_ENABLE      "enable"
#define ARG_COMMAND_DISABLE     "disable"
#define ARG_COMMAND_SESSION     "session"

#define HELP_OPTIONS        "Options:\n"\
"\t" ARG_HELP_LONG "\t\t\tDisplay this help.\n" \
"\t" ARG_HELP "\n" \
"\t" ARG_QUIET_LONG "\t\t\tDo not output debugging messages.\n" \
"\t" ARG_QUIET "\n" \
"\t" ARG_VERBOSE_LONG "\t\tIncrease output verbosity to level:\n" \
"\t" ARG_VERBOSE1 "\t\t\t1 - Info messages.\n" \
"\t" ARG_VERBOSE2 "\t\t\t2 - Debug messages.\n" \
"\t" ARG_VERBOSE3 "\t\t\t3 - Tracing messages.\n" \
""

#define QUEUE_MAX_LENGTH           10 // state
#define MAX_PROPERTY_LENGTH        512 //bytes

#define MAX_API_COUNT                   3

#define MAX_NOW_PLAYING_EVENTS          20

enum api_type {
    unknown = 0,
    lastfm,
    librefm,
    listenbrainz,
};

struct api_credentials {
    bool enabled;
    bool authenticated;
    const char *token;
    const char *api_key;
    const char *secret;
    const char *session_key;
    const char *url;
    char *user_name;
    char *password;
    enum api_type end_point;
};

struct env_variables {
    const char *user_name;
    const char *home;
    const char *xdg_config_home;
    const char *xdg_data_home;
    const char *xdg_cache_home;
    const char *xdg_runtime_dir;
};

struct configuration {
    const char *name;
#if 0
    const char *config_file;
    const char *credentials_file;
#endif
    struct env_variables *env;
    bool env_loaded;
    struct api_credentials *credentials[MAX_API_COUNT];
    size_t credentials_length;
};

struct mpris_metadata {
    char *album_artist;
    char *composer;
    char *genre;
    char *artist;
    char *comment;
    char *track_id;
    char *album;
    char *content_created;
    char *title;
    char *url;
    char *art_url; //mpris specific
    char *mb_track_id; //music brainz specific
    char *mb_album_id;
    char *mb_artist_id;
    char *mb_album_artist_id;
    uint64_t length; // mpris specific
    unsigned track_number;
    unsigned bitrate;
    unsigned disc_number;
    // TODO(marius): this does not belong here
    time_t timestamp;
};

struct mpris_properties {
    struct mpris_metadata *metadata;
    double volume;
    int64_t position;
    char *player_name;
    char *loop_status;
    char *playback_status;
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
    size_t now_playing_count;
    struct event *now_playing[MAX_NOW_PLAYING_EVENTS];
};

struct scrobble {
    char *title;
    char *album;
    char *artist;
    char *mb_track_id; //music brainz specific
    char *mb_album_id;
    char *mb_artist_id;
    char *mb_album_artist_id;
    char *mb_spotify_id; // spotify id for listenbrainz
    bool scrobbled;
    unsigned short track_number;
    unsigned length;
    time_t start_time;
    double play_time;
    double position;
};

enum playback_state {
    undetermined = 0,
    stopped = 1 << 0,
    paused = 1 << 1,
    playing = 1 << 2
};

struct mpris_event {
    enum playback_state player_state;
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
    struct scrobble *queue[QUEUE_MAX_LENGTH];
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
    none     = 0,
    error    = (1 << 0),
    warning  = (1 << 1),
    info     = (1 << 2),
    debug    = (1 << 3),
    tracing  = (1 << 4),
    tracing2 = (1 << 5),
};

enum binary_type {
    daemon_bin,
    signon_bin,
};

struct parsed_arguments {
    char *name;
    char *url;
    char *pid_path;
    bool has_url;
    bool has_help;
    bool get_token;
    bool get_session;
    bool disable;
    bool enable;
    enum binary_type binary;
    enum log_levels log_level;
    enum api_type service;
};

struct sighandler_payload {
    struct event_base *event_base;
    struct configuration *config;
};

struct now_playing_payload {
    // TODO(marius): this will be needed to free the event after we refactor the now_playing events array out of struct events
    //struct event_base *event_base;
    struct scrobbler *scrobbler;
    struct scrobble *track;
    struct events *events;
};

struct scrobble_payload {
    struct scrobbler *scrobbler;
    struct mpris_player *player;
    struct events *events;
};

#endif // MPRIS_SCROBBLER_STRUCTS_H
