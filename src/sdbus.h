/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include <event.h>
#include <dbus/dbus.h>

#define LOCAL_NAME                 "org.mpris.scrobbler"
#define MPRIS_PLAYER_NAMESPACE     "org.mpris.MediaPlayer2"
#define MPRIS_PLAYER_PATH          "/org/mpris/MediaPlayer2"
#define MPRIS_PLAYER_INTERFACE     "org.mpris.MediaPlayer2.Player"
#define MPRIS_METHOD_NEXT          "Next"
#define MPRIS_METHOD_PREVIOUS      "Previous"
#define MPRIS_METHOD_PLAY          "Play"
#define MPRIS_METHOD_PAUSE         "Pause"
#define MPRIS_METHOD_STOP          "Stop"
#define MPRIS_METHOD_PLAY_PAUSE    "PlayPause"

#define MPRIS_PNAME_PLAYBACKSTATUS "PlaybackStatus"
#define MPRIS_PNAME_CANCONTROL     "CanControl"
#define MPRIS_PNAME_CANGONEXT      "CanGoNext"
#define MPRIS_PNAME_CANGOPREVIOUS  "CanGoPrevious"
#define MPRIS_PNAME_CANPLAY        "CanPlay"
#define MPRIS_PNAME_CANPAUSE       "CanPause"
#define MPRIS_PNAME_CANSEEK        "CanSeek"
#define MPRIS_PNAME_SHUFFLE        "Shuffle"
#define MPRIS_PNAME_POSITION       "Position"
#define MPRIS_PNAME_VOLUME         "Volume"
#define MPRIS_PNAME_LOOPSTATUS     "LoopStatus"
#define MPRIS_PNAME_METADATA       "Metadata"

#define MPRIS_PROP_PLAYBACK_STATUS "PlaybackStatus"
#define MPRIS_PROP_METADATA        "Metadata"
#define MPRIS_ARG_PLAYER_IDENTITY  "Identity"

#define DBUS_DESTINATION           "org.freedesktop.DBus"
#define DBUS_PATH                  "/"
#define DBUS_INTERFACE             "org.freedesktop.DBus"
#define DBUS_PROPERTIES_INTERFACE  "org.freedesktop.DBus.Properties"
#define DBUS_METHOD_LIST_NAMES     "ListNames"
#define DBUS_METHOD_GET_ALL        "GetAll"
#define DBUS_METHOD_GET            "Get"

#define MPRIS_METADATA_BITRATE      "bitrate"
#define MPRIS_METADATA_ART_URL      "mpris:artUrl"
#define MPRIS_METADATA_LENGTH       "mpris:length"
#define MPRIS_METADATA_TRACKID      "mpris:trackid"
#define MPRIS_METADATA_ALBUM        "xesam:album"
#define MPRIS_METADATA_ALBUM_ARTIST "xesam:albumArtist"
#define MPRIS_METADATA_ARTIST       "xesam:artist"
#define MPRIS_METADATA_COMMENT      "xesam:comment"
#define MPRIS_METADATA_TITLE        "xesam:title"
#define MPRIS_METADATA_TRACK_NUMBER "xesam:trackNumber"
#define MPRIS_METADATA_URL          "xesam:url"
#define MPRIS_METADATA_YEAR         "year"

#define MPRIS_SIGNAL_PROPERTIES_CHANGED "PropertiesChanged"

#define MPRIS_PLAYBACK_STATUS_PLAYING  "Playing"
#define MPRIS_PLAYBACK_STATUS_PAUSED   "Paused"
#define MPRIS_PLAYBACK_STATUS_STOPPED  "Stopped"

// The default timeout leads to hangs when calling
//   certain players which don't seem to reply to MPRIS methods
#define DBUS_CONNECTION_TIMEOUT    100 //ms
#define MAX_PROPERTY_LENGTH        512 //bytes

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


typedef struct dbus_ctx {
    DBusConnection *conn;
    struct event_base *evbase;
    struct event dispatch_ev;
    mpris_properties *properties;
    void *extra;
} dbus_ctx;

void mpris_metadata_zero(mpris_metadata* metadata)
{
    metadata->track_number = 0;
    metadata->bitrate = 0;
    metadata->disc_number = 0;
    metadata->length = 0;
    metadata->content_created = 0;
#if 0
    zero_string(&metadata->album_artist, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->composer, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->genre, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->artist, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->comment, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->track_id, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->album, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->title, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->url, MAX_PROPERTY_LENGTH);
    zero_string(&metadata->art_url, MAX_PROPERTY_LENGTH);
#endif
    _log(tracing, "mem::zeroed_metadata(%p)", metadata);
}

void mpris_metadata_init(mpris_metadata* metadata)
{
    metadata->track_number = 0;
    metadata->bitrate = 0;
    metadata->disc_number = 0;
    metadata->length = 0;
    metadata->content_created = 0;
    metadata->album_artist = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->composer = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->genre = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->artist = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->comment = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->track_id = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->album = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->title = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->url = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    metadata->art_url = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    _log(tracing, "mem::inited_metadata(%p)", metadata);
}

void mpris_metadata_unref(void *data)
{
    mpris_metadata *metadata = data;
    _log(tracing, "mem::freeing_metadata(%p)", metadata);
#if 0
    if (NULL != metadata->album_artist) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->album_artist, metadata->album_artist);
        free(metadata->album_artist);
    }
    if (NULL != metadata->composer) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->composer, metadata->composer);
        free(metadata->composer);
    }
    if (NULL != metadata->genre) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->genre, metadata->genre);
        free(metadata->genre);
    }
    if (NULL != metadata->artist) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->artist, metadata->artist);
        free(metadata->artist);
    }
    if (NULL != metadata->comment) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->comment, metadata->comment);
        free(metadata->comment);
    }
    if (NULL != metadata->track_id) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->track_id, metadata->track_id);
        free(metadata->track_id);
    }
    if (NULL != metadata->album) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->album, metadata->album);
        free(metadata->album);
    }
    if (NULL != metadata->title) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->title, metadata->title);
        free(metadata->title);
    }
    if (NULL != metadata->url) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->url, metadata->url);
        free(metadata->url);
    }
    if (NULL != metadata->art_url) {
        _log(tracing, "mem:freeing_str(%p): %s", metadata->art_url, metadata->art_url);
        free(metadata->art_url);
    }
#endif
    if (NULL != metadata) {  free(metadata); }
}

void mpris_properties_zero(mpris_properties *properties)
{
    mpris_metadata_zero(properties->metadata);
    properties->volume = 0;
    properties->position = 0;
#if 0
    zero_string(&properties->player_name,MAX_PROPERTY_LENGTH);
    zero_string(&properties->loop_status, 20);
    zero_string(&properties->playback_status, 20);
#endif
    properties->can_control = false;
    properties->can_go_next = false;
    properties->can_go_previous = false;
    properties->can_play = false;
    properties->can_pause = false;
    properties->can_seek = false;
    properties->shuffle = false;
    _log(tracing, "mem::zeroed_properties(%p)", properties);
}

void mpris_properties_init(mpris_properties *properties)
{
    properties->metadata = malloc(sizeof(mpris_metadata));
    mpris_metadata_init(properties->metadata);
    properties->volume = 0;
    properties->position = 0;
    properties->player_name = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    properties->loop_status = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    properties->playback_status = NULL;//get_zero_string(MAX_PROPERTY_LENGTH);
    properties->can_control = false;
    properties->can_go_next = false;
    properties->can_go_previous = false;
    properties->can_play = false;
    properties->can_pause = false;
    properties->can_seek = false;
    properties->shuffle = false;
    _log(tracing, "mem::inited_properties(%p)", properties);
}

mpris_properties *mpris_properties_new()
{
    mpris_properties* properties = malloc(sizeof(mpris_properties));
    mpris_properties_init(properties);

    return properties;
}

void mpris_properties_unref(void *data)
{
    mpris_properties *properties = data;
    _log(tracing, "mem::freeing_properties(%p)", properties);
    mpris_metadata_unref(properties->metadata);
#if 0
    if (NULL != properties->player_name) {
        _log(tracing, "mem:freeing_str(%p): %s", properties->player_name, properties->player_name);
        free(properties->player_name);
    }
    if (NULL != properties->loop_status) {
        _log(tracing, "mem:freeing_str(%p): %s", properties->loop_status, properties->loop_status);
        free(properties->loop_status);
    }
    if (NULL != properties->playback_status) {
        _log(tracing, "mem:freeing_str(%p): %s", properties->playback_status, properties->playback_status);
        free(properties->playback_status);
    }
#endif
    if (NULL != properties) { free(properties); }
}

DBusMessage* call_dbus_method(DBusConnection* conn, char* destination, char* path, char* interface, char* method)
{
    if (NULL == conn) { return NULL; }
    if (NULL == destination) { return NULL; }

    DBusMessage* msg;
    DBusPendingCall* pending;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return NULL; }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // free message
    dbus_message_unref(msg);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);

    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);

    return reply;

_unref_message_err:
    {
        dbus_message_unref(msg);
    }
    return NULL;
}

double /*void */extract_double_var(DBusMessageIter *iter,/* double *result, */DBusError *error)
{
//    if (NULL == result) { return; }
    double result = 0.0f;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return result;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_DOUBLE == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
#if 0
        _log(tracing, "\tdbus::loaded_basic_var: %f", result);
#endif
    }
    return result;
}

char* /*void */extract_string_var(DBusMessageIter *iter,/*char** result, */DBusError *error)
{
//    if (NULL == result) { return; }
    char* result;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return NULL;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (
        DBUS_TYPE_OBJECT_PATH == dbus_message_iter_get_arg_type(&variantIter) ||
        DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&variantIter)
    ) {
        dbus_message_iter_get_basic(&variantIter, &result);
#if 0
        _log(tracing, "\tdbus::loaded_basic_var: %s", *result);
#endif
        return result;
    }
    if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&variantIter)) {
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&variantIter, &arrayIter);
        while (true) {
            // todo(marius): load all elements of the array
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayIter)) {
                dbus_message_iter_get_basic(&arrayIter, &result);
#if 0
                _log(tracing, "\tdbus::loaded_basic_var: %s", *result);
#endif
            }
            if (!dbus_message_iter_has_next(&arrayIter)) {
                break;
            }
            dbus_message_iter_next(&arrayIter);
        }
        return result;
    }
    return NULL;
}

int32_t /* void */ extract_int32_var(DBusMessageIter *iter, /*int32_t *result, */DBusError *error)
{
//    if (NULL == result) { return; }
    int32_t result = 0;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return result;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
#if 0
        _log(tracing, "\tdbus::loaded_basic_var: %ld", *result);
#endif
    }
    return result;
}

int64_t extract_int64_var(DBusMessageIter *iter, /*int64_t *result,*/ DBusError *error)
{
//    if (NULL == result) { return; }
    int64_t result = 0;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return result;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (
        DBUS_TYPE_UINT64 == dbus_message_iter_get_arg_type(&variantIter) ||
        DBUS_TYPE_INT64 == dbus_message_iter_get_arg_type(&variantIter)
    ) {
        dbus_message_iter_get_basic(&variantIter, &result);
#if 0
        _log(tracing, "\tdbus::loaded_basic_var: %lld", *result);
#endif
    }
    return result;
}

bool extract_boolean_var(DBusMessageIter *iter, /*bool *result,*/ DBusError *error)
{
//    if (NULL == result) { return; }
    bool result = false;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return false;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
#if 0
        _log(tracing, "\tdbus::loaded_basic_var: %s", *result ? "true" : "false");
#endif
    }
    return result;
}

void load_metadata(DBusMessageIter *iter, mpris_metadata *track)
{
    if (NULL == track) { return; }
    DBusError err;
    dbus_error_init(&err);

    //mpris_metadata_init(track);

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(&err, "iter_should_be_variant", "This message iterator must be have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(&err, "variant_should_be_array", "This variant reply message must have array content");
        return;
    }
    DBusMessageIter arrayIter;
    dbus_message_iter_recurse(&variantIter, &arrayIter);
    _log(info, "mpris::loading_metadata");
    while (true) {
        char* key;
        if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&arrayIter)) {
            DBusMessageIter dictIter;
            dbus_message_iter_recurse(&arrayIter, &dictIter);
            if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&dictIter)) {
                dbus_set_error_const(&err, "missing_key", "This message iterator doesn't have key");
            }
            dbus_message_iter_get_basic(&dictIter, &key);

            if (!dbus_message_iter_has_next(&dictIter)) {
                continue;
            }
            dbus_message_iter_next(&dictIter);
            if (!strncmp(key, MPRIS_METADATA_BITRATE, strlen(MPRIS_METADATA_BITRATE))) {
                track->bitrate = extract_int32_var(&dictIter, /*(int32_t *)&track->bitrate, */&err);
                _log (info, "  loaded::metadata:bitrate: %d", track->bitrate);
            }
            if (!strncmp(key, MPRIS_METADATA_ART_URL, strlen(MPRIS_METADATA_ART_URL))) {
                track->art_url = extract_string_var(&dictIter, /*&track->art_url, */&err);
                _log (info, "  loaded::metadata:art_url: %s", track->art_url);
            }
            if (!strncmp(key, MPRIS_METADATA_LENGTH, strlen(MPRIS_METADATA_LENGTH))) {
                track->length = extract_int64_var(&dictIter, /*(int64_t *)&track->length, */&err);
                _log (info, "  loaded::metadata:length: %" PRId64, track->length);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACKID, strlen(MPRIS_METADATA_TRACKID))) {
                track->track_id = extract_string_var(&dictIter, /*&track->track_id, */&err);
                _log (info, "  loaded::metadata:track_id: %s", track->track_id);
            }
            if (!strncmp(key, MPRIS_METADATA_ALBUM, strlen(MPRIS_METADATA_ALBUM)) && strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST))) {
                track->album = extract_string_var(&dictIter, /*&track->album, */&err);
                _log (info, "  loaded::metadata:album: %s", track->album);
            }
            if (!strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST))) {
                track->album_artist = extract_string_var(&dictIter, /*&track->album_artist, */&err);
                _log (info, "  loaded::metadata:album_artist: %s", track->album_artist);
            }
            if (!strncmp(key, MPRIS_METADATA_ARTIST, strlen(MPRIS_METADATA_ARTIST))) {
                track->artist = extract_string_var(&dictIter, /*&track->artist, */&err);
                _log (info, "  loaded::metadata:artist: %s", track->artist);
            }
            if (!strncmp(key, MPRIS_METADATA_COMMENT, strlen(MPRIS_METADATA_COMMENT))) {
                track->comment = extract_string_var(&dictIter, /*&track->comment, */&err);
                _log (info, "  loaded::metadata:comment: %s", track->comment);
            }
            if (!strncmp(key, MPRIS_METADATA_TITLE, strlen(MPRIS_METADATA_TITLE))) {
                track->title = extract_string_var(&dictIter, /*&track->title,*/ &err);
                _log (info, "  loaded::metadata:title: %s", track->title);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACK_NUMBER, strlen(MPRIS_METADATA_TRACK_NUMBER))) {
                track->track_number = extract_int32_var(&dictIter, /*(int32_t *)&track->track_number, */&err);
                _log (info, "  loaded::metadata:track_number: %2" PRId32, track->track_number);
            }
            if (!strncmp(key, MPRIS_METADATA_URL, strlen(MPRIS_METADATA_URL))) {
                track->url = extract_string_var(&dictIter, /*&track->url, */&err);
                _log (info, "  loaded::metadata:url: %s", track->url);
            }
            if (dbus_error_is_set(&err)) {
                _log(error, "dbus::value_error: %s, %s", key, err.message);
                dbus_error_free(&err);
            }
        }
        if (!dbus_message_iter_has_next(&arrayIter)) {
            break;
        }
        dbus_message_iter_next(&arrayIter);
    }
}

void get_player_identity(DBusConnection *conn, const char* destination, char **name)
{
    if (NULL == conn) { return; }
    if (NULL == destination) { return; }
    if (strncmp(MPRIS_PLAYER_NAMESPACE, destination, strlen(MPRIS_PLAYER_NAMESPACE))) { return; }

    DBusMessage* msg;
    DBusError err;
    DBusPendingCall* pending;
    DBusMessageIter params;

    char* interface = DBUS_PROPERTIES_INTERFACE;
    char* method = DBUS_METHOD_GET;
    char* path = MPRIS_PLAYER_PATH;
    char* arg_interface = MPRIS_PLAYER_NAMESPACE;
    char* arg_identity = MPRIS_ARG_PLAYER_IDENTITY;

    dbus_error_init(&err);
    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return; }

    // append interface we want to get the property from
    dbus_message_iter_init_append(msg, &params);
    if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, &arg_interface)) {
        goto _unref_message_err;
    }

    dbus_message_iter_init_append(msg, &params);
    if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, &arg_identity)) {
        goto _unref_message_err;
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) { goto _unref_pending_err; }

    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter)) {
        *name = extract_string_var(&rootIter, /*name, */&err);
    }
    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
    }

    dbus_message_unref(reply);
    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);
    if (NULL != *name) {
        _log (info, "  loaded::player_name: %s", *name);
    }

    return;

_unref_pending_err:
    {
        dbus_pending_call_unref(pending);
        goto _unref_message_err;
    }
_unref_message_err:
    {
        dbus_message_unref(msg);
    }
    return;
}

char* get_dbus_string_scalar(DBusMessage* message)
{
    if (NULL == message) { return NULL; }
    char* status = NULL;

    DBusMessageIter rootIter;
    if (dbus_message_iter_init(message, &rootIter) &&
        DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&rootIter)) {

        dbus_message_iter_get_basic(&rootIter, &status);
    }

    return status;
}

void get_player_namespace(DBusConnection* conn, char** player_namespace)
{
    if (NULL == conn) { return; }
    if (NULL == player_namespace) { return; }

    const char* method = DBUS_METHOD_LIST_NAMES;
    const char* destination = DBUS_DESTINATION;
    const char* path = DBUS_PATH;
    const char* interface = DBUS_INTERFACE;
    const char* mpris_namespace = MPRIS_PLAYER_NAMESPACE;
    size_t namespace_len = 0;

    if (NULL != *player_namespace) {
        namespace_len = strlen(*player_namespace);
    }

    DBusMessage* msg;
    DBusPendingCall* pending;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return; }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) { goto _unref_pending_err; }

    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter) &&
        DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&rootIter)) {
        DBusMessageIter arrayElementIter;

        dbus_message_iter_recurse(&rootIter, &arrayElementIter);
        while (true) {
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayElementIter)) {
                char* str;
                dbus_message_iter_get_basic(&arrayElementIter, &str);
                if (!strncmp(str, mpris_namespace, strlen(mpris_namespace))) {
                    strncpy(*player_namespace, str, strlen(str));
                    break;
                }
            }
            if (!dbus_message_iter_has_next(&arrayElementIter)) {
                if (NULL != *player_namespace) { zero_string(player_namespace, namespace_len); }
                break;
            }
            dbus_message_iter_next(&arrayElementIter);
        }
    }
    dbus_message_unref(reply);
    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);
    return;

_unref_pending_err:
    {
        dbus_pending_call_unref(pending);
        goto _unref_message_err;
    }
_unref_message_err:
    {
        dbus_message_unref(msg);
    }
}

void load_properties(DBusMessageIter *rootIter, mpris_properties *properties)
{
    if (NULL == properties) { return; }
    if (NULL == rootIter) { return; }

    DBusError err;
    dbus_error_init(&err);

    if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(rootIter)) {
        DBusMessageIter arrayElementIter;

        dbus_message_iter_recurse(rootIter, &arrayElementIter);
        while (true) {
            char* key;
            if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&arrayElementIter)) {
                DBusMessageIter dictIter;
                dbus_message_iter_recurse(&arrayElementIter, &dictIter);
                if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&dictIter)) {
                    dbus_set_error_const(&err, "missing_key", "This message iterator doesn't have key");
                }
                dbus_message_iter_get_basic(&dictIter, &key);

                if (!dbus_message_iter_has_next(&dictIter)) {
                    continue;
                }
                dbus_message_iter_next(&dictIter);

                if (!strncmp(key, MPRIS_PNAME_CANCONTROL, strlen(MPRIS_PNAME_CANCONTROL))) {
                      properties->can_control = extract_boolean_var(&dictIter, /*&properties->can_control, */&err);
                     _log (info, "  loaded::can_control: %s", (properties->can_control ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_CANGONEXT, strlen(MPRIS_PNAME_CANGONEXT))) {
                     properties->can_go_next = extract_boolean_var(&dictIter, /*&properties->can_go_next, */&err);
                     _log (info, "  loaded::can_go_next: %s", (properties->can_go_next ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_CANGOPREVIOUS, strlen(MPRIS_PNAME_CANGOPREVIOUS))) {
                   properties->can_go_previous = extract_boolean_var(&dictIter, /*&properties->can_go_previous, */&err);
                   _log (info, "  loaded::can_go_previous: %s", (properties->can_go_previous ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_CANPAUSE, strlen(MPRIS_PNAME_CANPAUSE))) {
                    properties->can_pause = extract_boolean_var(&dictIter, /*&properties->can_pause, */&err);
                   _log (info, "  loaded::can_pause: %s", (properties->can_pause ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_CANPLAY, strlen(MPRIS_PNAME_CANPLAY))) {
                    properties->can_play = extract_boolean_var(&dictIter, /*&properties->can_play, */&err);
                   _log (info, "  loaded::can_play: %s", (properties->can_play ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_CANSEEK, strlen(MPRIS_PNAME_CANSEEK))) {
                    properties->can_seek = extract_boolean_var(&dictIter, /*&properties->can_seek,*/ &err);
                   _log (info, "  loaded::can_seek: %s", (properties->can_seek ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_LOOPSTATUS, strlen(MPRIS_PNAME_LOOPSTATUS))) {
                    properties->loop_status = extract_string_var(&dictIter, /*&properties->loop_status, */&err);
                   _log (info, "  loaded::loop_status: %s", properties->loop_status);
                }
                if (!strncmp(key, MPRIS_PNAME_METADATA, strlen(MPRIS_PNAME_METADATA))) {
                     load_metadata(&dictIter, properties->metadata);
                }
                if (!strncmp(key, MPRIS_PNAME_PLAYBACKSTATUS, strlen(MPRIS_PNAME_PLAYBACKSTATUS))) {
                    properties->playback_status = extract_string_var(&dictIter, /*&properties->playback_status, */&err);
                    _log (info, "  loaded::playback_status: %s", properties->playback_status);
                }
                if (!strncmp(key, MPRIS_PNAME_POSITION, strlen(MPRIS_PNAME_POSITION))) {
                     properties->position = extract_int64_var(&dictIter, /*&properties->position, */&err);
                    _log (info, "  loaded::position: %" PRId64, properties->position);
                }
                if (!strncmp(key, MPRIS_PNAME_SHUFFLE, strlen(MPRIS_PNAME_SHUFFLE))) {
                     properties->shuffle = extract_boolean_var(&dictIter, /*&properties->shuffle, */&err);
                    _log (info, "  loaded::shuffle: %s", (properties->shuffle ? "yes" : "no"));
                }
                if (!strncmp(key, MPRIS_PNAME_VOLUME, strlen(MPRIS_PNAME_VOLUME))) {
                     properties->volume = extract_double_var(&dictIter, /*&properties->volume, */&err);
                    _log (info, "  loaded::volume: %.2f", properties->volume);
                }
                if (dbus_error_is_set(&err)) {
                    _log(error, "dbus::value_error: %s", err.message);
                    dbus_error_free(&err);
                }
            }
            if (!dbus_message_iter_has_next(&arrayElementIter)) {
                break;
            }
            dbus_message_iter_next(&arrayElementIter);
        }
    }
}

bool mpris_player_is_valid(char** name)
{
    return (NULL != name && NULL != *name && strlen(*name) > 0);
}

bool mpris_properties_are_loaded(const mpris_properties *p)
{
    bool result = false;

    result = (NULL != p->metadata->title && NULL != p->metadata->artist && NULL != p->metadata->album);

    return result;
}

void get_mpris_properties(DBusConnection* conn, const char* destination, mpris_properties *properties)
{
    if (NULL == properties) { return; }
    if (NULL == conn) { return; }
    if (NULL == destination) { return; }

    DBusMessage* msg;
    DBusPendingCall* pending;
    DBusMessageIter params;

    char* interface = DBUS_PROPERTIES_INTERFACE;
    char* method = DBUS_METHOD_GET_ALL;
    char* path = MPRIS_PLAYER_PATH;
    char* arg_interface = MPRIS_PLAYER_INTERFACE;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return; }

    // append interface we want to get the property from
    dbus_message_iter_init_append(msg, &params);
    if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, &arg_interface)) {
        goto _unref_message_err;
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);
    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusError err;
    dbus_error_init(&err);

    DBusMessage* reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) {
        goto _unref_pending_err;
    }
    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter) && DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&rootIter)) {
        _log(info, "mpris::loading_properties");
        mpris_properties_zero(properties);
        load_properties(&rootIter, properties);
    }
    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
    }
    dbus_message_unref(reply);
    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);

    get_player_identity(conn, destination, &properties->player_name);
    return;

_unref_pending_err:
    {
        dbus_pending_call_unref(pending);
        goto _unref_message_err;
    }
_unref_message_err:
    {
        dbus_message_unref(msg);
    }
}

void check_for_player(DBusConnection *conn, char **destination, time_t *last_load_time)
{
    if (NULL == conn) { return; }
    time_t current_time;
    bool valid_incoming_player = mpris_player_is_valid(destination);
    time(&current_time);

    if (difftime(current_time, *last_load_time) < 10 && valid_incoming_player) { return; }

    get_player_namespace(conn, destination);
    time(last_load_time);

    if (mpris_player_is_valid(destination)) {
        if (!valid_incoming_player) { _log(debug, "mpris::found_player: %s", *destination); }
    } else {
        if (valid_incoming_player) { _log(debug, "mpris::lost_player"); }
    }
}

inline bool mpris_properties_is_playing(const mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PLAYING, strlen(MPRIS_PLAYBACK_STATUS_PLAYING)) == 0
    );
}

inline bool mpris_properties_is_paused(const mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_STOPPED, strlen(MPRIS_PLAYBACK_STATUS_STOPPED)) == 0
    );
}

inline bool mpris_properties_is_stopped(const mpris_properties *s)
{
    return (
        (NULL != s) &&
        (NULL != s->playback_status) &&
        strncmp(s->playback_status, MPRIS_PLAYBACK_STATUS_PAUSED, strlen(MPRIS_PLAYBACK_STATUS_PAUSED)) == 0
    );
}

DBusHandlerResult load_properties_from_message(DBusMessage *msg, mpris_properties *data)
{
    if (NULL == msg) {
        _log(warning, "dbus::invalid_signal_message(%p)", msg);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    if (NULL != data) {
        DBusMessageIter args;
        // read the parameters
        const char* signal_name = dbus_message_get_member(msg);
        if (!dbus_message_iter_init(msg, &args)) {
            _log(warning, "dbus::missing_signal_args: %s", signal);
        }
        _log (debug, "dbus::signal(%p): %s:%s::%s", msg, dbus_message_get_path(msg), dbus_message_get_interface(msg), signal_name);
        // skip first arg and then load properties from all the remaining ones
        while(dbus_message_iter_next(&args)) { load_properties(&args, data); }
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}

void dispatch(int fd, short ev, void *data)
{
    dbus_ctx *ctx = data;
    DBusConnection *conn = ctx->conn;

    _log(tracing,"dbus::dispatching fd=%d, data=%p ev=%d", fd, (void*)data, ev);
    while (dbus_connection_get_dispatch_status(conn) == DBUS_DISPATCH_DATA_REMAINS) {
        dbus_connection_dispatch(conn);
    }
}

void handle_dispatch_status(DBusConnection *conn, DBusDispatchStatus status, void *data)
{
    dbus_ctx *ctx = data;

    if (status == DBUS_DISPATCH_DATA_REMAINS) {
        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 0,
        };
        event_add (&ctx->dispatch_ev, &tv);
        _log(tracing,"dbus::new_dispatch_status(%p): %s", (void*)conn, "DATA_REMAINS");
    }
    if (status == DBUS_DISPATCH_COMPLETE) {
        _log(tracing,"dbus::new_dispatch_status(%p): %s", (void*)conn, "COMPLETE");
    }
    if (status == DBUS_DISPATCH_NEED_MEMORY) {
        _log(tracing,"dbus::new_dispatch_status(%p): %s", (void*)conn, "OUT_OF_MEMORY");
    }
}

void handle_watch(int fd, short events, void *data)
{
    dbus_ctx *ctx = data;

    DBusWatch *watch = ctx->extra;

    unsigned flags = 0;

    if (events & EV_READ) { flags |= DBUS_WATCH_READABLE; }

    _log(tracing,"dbus::handle_event: fd=%d, watch=%p ev=%d", fd, (void*)watch, events);
    if (dbus_watch_handle(watch, flags) == false) {
       _log(error,"dbus::handle_event_failed: fd=%d, watch=%p ev=%d", fd, (void*)watch, events);
    }

    handle_dispatch_status(ctx->conn, DBUS_DISPATCH_DATA_REMAINS, ctx);
}

unsigned add_watch(DBusWatch *watch, void *data)
{
    if (!dbus_watch_get_enabled(watch)) { return true;}

    dbus_ctx *ctx = data;
    ctx->extra = watch;

    int fd = dbus_watch_get_unix_fd(watch);
    unsigned flags = dbus_watch_get_flags(watch);

    short cond = EV_PERSIST;
    if (flags & DBUS_WATCH_READABLE) { cond |= EV_READ; }

    struct event *event = event_new(ctx->evbase, fd, cond, handle_watch, ctx);

    if (NULL == event) { return false; }
    event_add(event, NULL);

    dbus_watch_set_data(watch, event, NULL);

    _log(tracing,"dbus::added_watch: watch=%p data=%p", (void*)watch, data);
    return true;
}

void remove_watch(DBusWatch *watch, void *data)
{
    if (!dbus_watch_get_enabled(watch)) { return; }

    struct event *event = dbus_watch_get_data(watch);

    if (NULL != event) { free(event); }

    dbus_watch_set_data(watch, NULL, NULL);

    _log(tracing,"dbus::removed_watch: watch=%p data=%p", (void*)watch, data);
}

void toggle_watch(DBusWatch *watch, void *data)
{
    _log(tracing,"dbus::toggle_watch watch=%p data=%p", (void*)watch, data);

    if (dbus_watch_get_enabled(watch)) {
        add_watch(watch, data);
    } else {
        remove_watch(watch, data);
    }
}

DBusHandlerResult handle_signal_message(DBusMessage *message, void *data)
{
    _log(tracing,"dbus::handling_signal: message=%p data=%p", (void*)message, (void*)data);
    dbus_ctx *ctx = data;
    mpris_properties* p = ctx->properties;

    return load_properties_from_message(message, p);
}

DBusHandlerResult add_filter(DBusConnection *conn, DBusMessage *message, void *data)
{
    dbus_ctx *ctx = data;
    if (dbus_message_is_signal(message, DBUS_PROPERTIES_INTERFACE, MPRIS_SIGNAL_PROPERTIES_CHANGED)) {
        _log(tracing,"dbus::filter: received recognized signal on conn=%p", (void*)conn);
        mpris_properties* p = ctx->properties;

        //return handle_signal_message(message, data);
        return load_properties_from_message(message, p);
    } else {
        _log(tracing,"dbus::filter:unknown_message %d %s -> %s %s/%s/%s %s",
               dbus_message_get_type(message),
               dbus_message_get_sender(message),
               dbus_message_get_destination(message),
               dbus_message_get_path(message),
               dbus_message_get_interface(message),
               dbus_message_get_member(message),
               dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR ?
               dbus_message_get_error_name(message) : "");
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void remove_filter(DBusConnection *conn, void *data)
{
    _log(tracing,"dbus::remove_filter: conn=%p, data=%p", (void*)conn, (void*)data);
}

void dbus_close(dbus_ctx *ctx)
{
    _log(tracing, "mem::cleanup");
    if (NULL == ctx) { return; }
    if (NULL != ctx->conn) {
        dbus_connection_flush(ctx->conn);
        dbus_connection_close(ctx->conn);
        dbus_connection_unref(ctx->conn);

        _log(tracing, "mem::freed_dbus_connection");
    }
    if (NULL != &ctx->dispatch_ev) {
        event_del(&ctx->dispatch_ev);
        _log(tracing, "mem::freed_dispatch_event");
    }
    if (NULL != ctx->evbase) {
        event_base_free(ctx->evbase);
        _log(tracing, "mem::freed_base_libevent");
    }
    if (NULL != ctx->properties) {
        mpris_properties_unref(ctx->properties);
        _log(tracing, "mem::freed_properties");
    }
    free(ctx);
}

void handle_timeout(int fd, short ev, void *data)
{
    dbus_ctx *ctx = data;

    DBusTimeout *t = ctx->extra;

    _log(tracing,"dbus::timeout_reached: fd=%d ev=%p events=%d", fd, (void*)t, ev);

    dbus_timeout_handle(t);
}

unsigned add_timeout(DBusTimeout *t, void *data)
{
    dbus_ctx *ctx = data;

    if (!dbus_timeout_get_enabled(t)) { return 1; }

    _log(tracing,"dbus::add_timeout: %p data=%p", (void*)t, data);
    struct event *event = event_new(ctx->evbase, -1, EV_TIMEOUT|EV_PERSIST, handle_timeout, t);
    if (NULL == event) {
        _log(tracing,"dbus::add_timeout: failed");
    }

    int ms = dbus_timeout_get_interval(t);
    struct timeval tv = {
        .tv_sec = ms / 1000,
        .tv_usec = (ms % 1000) * 1000,
    };
    event_add(event, &tv);
    dbus_timeout_set_data(t, event, NULL);

    return 1;
}

void remove_timeout(DBusTimeout *t, void *data)
{
    struct event *event = dbus_timeout_get_data(t);

    _log(tracing,"dbus::remove_timeout: %p data=%p event=%p", (void*)t, data, (void*)event);

    event_free(event);

    dbus_timeout_set_data(t, NULL, NULL);
}

void toggle_timeout(DBusTimeout *t, void *data)
{
    _log(tracing,"dbus::toggle_timeout: %p", (void*)t);
    if (dbus_timeout_get_enabled(t)) {
        add_timeout(t, data);
    } else {
        remove_timeout(t, data);
    }
}

dbus_ctx *dbus_connection_init(struct event_base *eb, mpris_properties* properties)
{
    DBusConnection *conn = NULL;
    dbus_ctx *ctx = calloc(1, sizeof(dbus_ctx));
    if (NULL == ctx) {
        _log(error, "dbus::failed_to_init_libdbus");
        goto _cleanup;
    }
    DBusError err;
    dbus_error_init(&err);

    conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (NULL == conn) {
        _log(error, "dbus::unable_to_get_on_bus");
        goto _cleanup;
    }
    dbus_connection_set_exit_on_disconnect(conn, false);

    ctx->conn = conn;
    ctx->evbase = eb;
    ctx->properties = properties;
    event_assign(&ctx->dispatch_ev, eb, -1, EV_TIMEOUT, dispatch, ctx);

    if (dbus_connection_set_watch_functions(conn, add_watch, remove_watch, toggle_watch, ctx, NULL) == false) {
        _log(error, "dbus::add_watch_functions: failed");
        goto _cleanup;
    }

    if (dbus_connection_add_filter(conn, add_filter, ctx, NULL) == false) {
        _log(error, "dbus::connection_add_filter: failed");
        goto _cleanup;
    }

    //if (dbus_connection_set_timeout_functions(conn, add_timeout, remove_timeout, toggle_timeout, ctx, NULL) == false) {
    //    _log(error, "dbus::add_timout_functions: failed");
    //    goto _cleanup;
    //}

    dbus_connection_set_dispatch_status_function(conn, handle_dispatch_status, ctx, NULL);

    const char* signal_sig = "type='signal',interface='" DBUS_PROPERTIES_INTERFACE "'";
    dbus_bus_add_match(conn, signal_sig, &err);

    if (dbus_error_is_set(&err)) {
        _log(error, "dbus::add_signal: %s", err.message);
        dbus_error_free(&err);
        goto _cleanup;
    }

    return ctx;
_cleanup:
    if (dbus_error_is_set(&err)) {
        _log(tracing,"dbus::err: %s", err.message);
        dbus_error_free(&err);
    }
    dbus_close(ctx);
    return NULL;
}
