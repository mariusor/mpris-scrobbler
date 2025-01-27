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

#define MAX_PROPERTY_LENGTH           384 // bytes

#define MAX_API_COUNT                   3

#define MAX_NOW_PLAYING_EVENTS         20

enum end_point_type {
    unknown_endpoint = 0,
    authorization_endpoint,
    scrobble_endpoint,
};

enum api_type {
    api_unknown = 0,
    api_lastfm,
    api_librefm,
    api_listenbrainz,
};

#define MAX_SECRET_LENGTH 128
struct api_credentials {
    const char *api_key;
    const char *secret;
    const char *url;
    char *user_name;
    char *password;
    enum api_type end_point;
    bool enabled;
    bool authenticated;
    const char token[MAX_SECRET_LENGTH + 1];
    const char session_key[MAX_SECRET_LENGTH + 1];
};

#define USER_NAME_MAX MAX_PROPERTY_LENGTH / 3
#define HOME_MAX MAX_PROPERTY_LENGTH - 30
#define FILE_PATH_MAX 4095
#define ENV_PATH_MAX 511

struct env_variables {
    const char user_name[USER_NAME_MAX + 1];
    const char home[HOME_MAX + 1];
    const char xdg_config_home[ENV_PATH_MAX + 1];
    const char xdg_data_home[ENV_PATH_MAX + 1];
    const char xdg_cache_home[ENV_PATH_MAX + 1];
    const char xdg_runtime_dir[ENV_PATH_MAX + 1];
};

#define MAX_PLAYERS 10
#define MAX_NAME_LENGTH 47

struct configuration {
    struct api_credentials **credentials;
    struct env_variables env;
    bool wrote_pid;
    bool env_loaded;
    short ignore_players_count;
    const char name[MAX_NAME_LENGTH+1];
    const char pid_path[FILE_PATH_MAX+1];
    const char config_path[FILE_PATH_MAX+1];
    const char credentials_path[FILE_PATH_MAX+1];
    const char cache_path[FILE_PATH_MAX+1];
    const char ignore_players[MAX_PLAYERS][MAX_PROPERTY_LENGTH+1];
};

#define MAX_PROPERTY_COUNT 8
struct mpris_metadata {
    uint64_t length; // mpris specific
    unsigned track_number;
    unsigned bitrate;
    unsigned disc_number;
    char track_id[MAX_PROPERTY_LENGTH+1];
    char album[MAX_PROPERTY_LENGTH+1];
    char content_created[MAX_PROPERTY_LENGTH+1];
    char title[MAX_PROPERTY_LENGTH+1];
    char url[MAX_PROPERTY_LENGTH+1];
    char art_url[MAX_PROPERTY_LENGTH+1]; //mpris specific
    char composer[MAX_PROPERTY_LENGTH+1];
    char genre[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1];
    char comment[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1];
    char artist[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1];
    char album_artist[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1];
    char mb_track_id[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1]; //music brainz specific
    char mb_album_id[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1];
    char mb_artist_id[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1];
    char mb_album_artist_id[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1];
};

struct mpris_properties {
    struct mpris_metadata metadata;
    double volume;
    int64_t position;
    bool can_control;
    bool can_go_next;
    bool can_go_previous;
    bool can_play;
    bool can_pause;
    bool can_seek;
    bool shuffle;
    char player_name[MAX_PROPERTY_LENGTH+1];
    char loop_status[MAX_PROPERTY_LENGTH+1];
    char playback_status[MAX_PROPERTY_LENGTH+1];
};

struct events {
    struct event_base *base;
    struct event *sigint;
    struct event *sigterm;
    struct event *sighup;
    struct event dispatch;
};

struct scrobble {
    double play_time;
    double position;
    double length;
    time_t start_time;

    bool scrobbled;
    unsigned short track_number;

    char url[MAX_PROPERTY_LENGTH+1];
    char title[MAX_PROPERTY_LENGTH+1];
    char album[MAX_PROPERTY_LENGTH+1];
    char artist[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1];

    char mb_track_id[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1]; //music brainz specific
    char mb_album_id[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1];
    char mb_artist_id[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1];
    char mb_album_artist_id[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH+1];
    char player_name[MAX_PROPERTY_LENGTH+1];
    char mb_spotify_id[MAX_PROPERTY_LENGTH+1]; // spotify id for listenbrainz
};

enum playback_state {
    killed = 0U,
    stopped = 1U << 0U,
    paused = 1U << 1U,
    playing = 1U << 2U
};

enum mpris_load_types {
    mpris_load_nothing = 0U,
    mpris_load_property_can_control = 1U << 0U,
    mpris_load_property_can_go_next = 1U << 1U,
    mpris_load_property_can_go_previous = 1U << 2U,
    mpris_load_property_can_pause = 1U << 3U,
    mpris_load_property_can_play = 1U << 4U,
    mpris_load_property_can_seek = 1U << 5U,
    mpris_load_property_loop_status = 1U << 6U,
    mpris_load_property_volume = 1U << 7U,
    mpris_load_property_shuffle = 1U << 8U,
    // from here the loaded information is relevant for a scrobble change
    mpris_load_property_position = 1U << 9U,
    mpris_load_property_playback_status = 1U << 10U,
    mpris_load_metadata_bitrate = 1U << 11U,
    mpris_load_metadata_art_url = 1U << 12U,
    mpris_load_metadata_length = 1U << 13U,
    mpris_load_metadata_track_id = 1U << 14U,
    mpris_load_metadata_album = 1U << 15U,
    mpris_load_metadata_album_artist = 1U << 16U,
    mpris_load_metadata_artist = 1U << 17U,
    mpris_load_metadata_comment = 1U << 18U,
    mpris_load_metadata_title = 1U << 19U,
    mpris_load_metadata_track_number = 1U << 20U,
    mpris_load_metadata_url = 1U << 21U,
    mpris_load_metadata_genre = 1U << 22U,
    mpris_load_metadata_mb_track_id = 1U << 23U,
    mpris_load_metadata_mb_album_id = 1U << 24U,
    mpris_load_metadata_mb_artist_id = 1U << 25U,
    mpris_load_metadata_mb_album_artist_id = 1U << 26U,
    mpris_load_all = (1U << 31U) - 1, // all bits are set for our max enum val
};

struct mpris_event {
    time_t timestamp;
    enum playback_state player_state;
    bool playback_status_changed;
    bool track_changed;
    bool volume_changed;
    bool position_changed;
    long loaded_state;
    char sender_bus_id[MAX_PROPERTY_LENGTH+1];
};

struct dbus {
    DBusConnection *conn;
    DBusWatch *watch;
    DBusTimeout *timeout;
};

struct event_payload {
    // either the scrobbler or the player
    void *parent;
    struct scrobble scrobble;
    struct event event;
};

#define MAX_QUEUE_LENGTH 63

struct scrobble_connections {
    int length;
    struct scrobbler_connection *entries[MAX_QUEUE_LENGTH+1];
};

struct scrobble_queue {
    int length;
    struct scrobble entries[MAX_QUEUE_LENGTH+1];
};

struct scrobbler {
    int still_running;
    CURLM *handle;
    struct event_base *evbase;
    struct configuration *conf;
    struct event timer_event;
    struct scrobble_connections connections;
    struct scrobble_queue queue;
};

struct mpris_player {
    bool ignored;
    bool deleted;
    char mpris_name[MAX_PROPERTY_LENGTH + 1];
    char bus_id[MAX_PROPERTY_LENGTH + 1];
    char name[MAX_PROPERTY_LENGTH + 1];
    struct mpris_event changed;
    struct mpris_properties properties;
    struct event_payload now_playing;
    struct event_payload queue;
    struct scrobbler *scrobbler;
    struct event_base *evbase;
    struct mpris_properties **history;
};

struct state {
    struct scrobbler scrobbler;
    struct dbus *dbus;
    struct configuration *config;
    struct events events;
    short player_count;
    struct mpris_player players[MAX_PLAYERS];
};

enum log_levels
{
    log_none     = 0U,
    log_error    = (1U << 0U),
    log_warning  = (1U << 1U),
    log_info     = (1U << 2U),
    log_debug    = (1U << 3U),
    log_tracing  = (1U << 4U),
    log_tracing2 = (1U << 5U),
};

enum binary_type {
    daemon_bin,
    signon_bin,
};

struct parsed_arguments {
    char *name;
    char *url;
    bool has_url;
    bool has_help;
    bool get_token;
    bool get_session;
    bool disable;
    bool enable;
    enum binary_type binary;
    enum log_levels log_level;
    enum api_type service;
    char pid_path[MAX_PROPERTY_LENGTH + 1];
};

struct scrobbler_connection {
    struct event ev;
    struct event retry_event;
    struct api_credentials credentials;
    struct scrobbler *parent;
    CURL *handle;
    struct curl_slist **headers;
    struct http_request *request;
    struct http_response *response;
    curl_socket_t sockfd;
    int action;
    int idx;
    int retries;
    char error[CURL_ERROR_SIZE];
};

#endif // MPRIS_SCROBBLER_STRUCTS_H
