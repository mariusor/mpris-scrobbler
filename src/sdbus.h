/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef SDBUS_H
#define SDBUS_H
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

// The default timeout leads to hangs when calling
//   certain players which don't seem to reply to MPRIS methods
#define DBUS_CONNECTION_TIMEOUT    100 //ms

#if 0
static DBusMessage* call_dbus_method(DBusConnection* conn, char* destination, char* path, char* interface, char* method)
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
#endif

static double extract_double_var(DBusMessageIter *iter, DBusError *error)
{
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

static char* extract_string_var(DBusMessageIter *iter, DBusError *error)
{
    char* result = NULL;
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
        char *temp = NULL;
        dbus_message_iter_get_basic(&variantIter, &temp);
        if (NULL != temp) {
            size_t len = strlen(temp);
            result = get_zero_string(len);
            strncpy(result, temp, len+1);
        }
#if 0
        _log(tracing, "\tdbus::loaded_basic_var: %s", result);
#endif
        return result;
    }
    if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&variantIter)) {
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&variantIter, &arrayIter);
        result = get_zero_string(2 * MAX_PROPERTY_LENGTH);
        size_t r_len = 0;

        while (true) {
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayIter)) {
                char *temp = NULL;
                dbus_message_iter_get_basic(&arrayIter, &temp);
                if (NULL != temp) {
                    if (NULL != result) { r_len = strlen(result); }
                    size_t len = strlen(temp);
                    if ((r_len + len) <= (2 * MAX_PROPERTY_LENGTH)) {
                        strncpy(result+r_len, temp, len);
                    }
                }
#if 0
                _log(tracing, "\tdbus::loaded_basic_var: %s", result);
#endif
            }
            if (!dbus_message_iter_has_next(&arrayIter)) {
                break;
            }
            dbus_message_iter_next(&arrayIter);
        }
    }
    return result;
}

static int32_t extract_int32_var(DBusMessageIter *iter, DBusError *error)
{
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

static int64_t extract_int64_var(DBusMessageIter *iter, DBusError *error)
{
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

static bool extract_boolean_var(DBusMessageIter *iter, DBusError *error)
{
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

static void load_metadata(DBusMessageIter *iter, mpris_metadata *track)
{
    if (NULL == track) { return; }
    DBusError err;
    dbus_error_init(&err);

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
    while (dbus_message_iter_has_next(&arrayIter)) {
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
                track->bitrate = extract_int32_var(&dictIter, &err);
                _log (info, "  loaded::metadata:bitrate: %d", track->bitrate);
            }
            if (!strncmp(key, MPRIS_METADATA_ART_URL, strlen(MPRIS_METADATA_ART_URL))) {
                track->art_url = extract_string_var(&dictIter, &err);
                _log (info, "  loaded::metadata:art_url: %s", track->art_url);
            }
            if (!strncmp(key, MPRIS_METADATA_LENGTH, strlen(MPRIS_METADATA_LENGTH))) {
                track->length = extract_int64_var(&dictIter, &err);
                _log (info, "  loaded::metadata:length: %" PRId64, track->length);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACKID, strlen(MPRIS_METADATA_TRACKID))) {
                track->track_id = extract_string_var(&dictIter, &err);
                _log (info, "  loaded::metadata:track_id: %s", track->track_id);
            }
            if (!strncmp(key, MPRIS_METADATA_ALBUM, strlen(MPRIS_METADATA_ALBUM)) && strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST)) ) {
                track->album = extract_string_var(&dictIter, &err);
                _log (info, "  loaded::metadata:album: %s", track->album);
            }
            if (!strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST))) {
                track->album_artist = extract_string_var(&dictIter, &err);
                _log (info, "  loaded::metadata:album_artist: %s", track->album_artist);
            }
            if (!strncmp(key, MPRIS_METADATA_ARTIST, strlen(MPRIS_METADATA_ARTIST))) {
                track->artist = extract_string_var(&dictIter, &err);
                _log (info, "  loaded::metadata:artist: %s", track->artist);
            }
            if (!strncmp(key, MPRIS_METADATA_COMMENT, strlen(MPRIS_METADATA_COMMENT))) {
                track->comment = extract_string_var(&dictIter, &err);
                _log (info, "  loaded::metadata:comment: %s", track->comment);
            }
            if (!strncmp(key, MPRIS_METADATA_TITLE, strlen(MPRIS_METADATA_TITLE))) {
                track->title = extract_string_var(&dictIter, &err);
                _log (info, "  loaded::metadata:title: %s", track->title);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACK_NUMBER, strlen(MPRIS_METADATA_TRACK_NUMBER))) {
                track->track_number = extract_int32_var(&dictIter, &err);
                _log (info, "  loaded::metadata:track_number: %2" PRId32, track->track_number);
            }
            if (!strncmp(key, MPRIS_METADATA_URL, strlen(MPRIS_METADATA_URL))) {
                track->url = extract_string_var(&dictIter, &err);
                _log (info, "  loaded::metadata:url: %s", track->url);
            }
            if (dbus_error_is_set(&err)) {
                _log(error, "dbus::value_error: %s, %s", key, err.message);
                dbus_error_free(&err);
            }
        }
        dbus_message_iter_next(&arrayIter);
    }
}

static char* get_player_identity(DBusConnection *conn, const char* destination)
{
    if (NULL == conn) { return NULL; }
    if (NULL == destination) { return NULL; }
    if (strncmp(MPRIS_PLAYER_NAMESPACE, destination, strlen(MPRIS_PLAYER_NAMESPACE)) != 0) { return NULL; }

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
    if (NULL == msg) { return NULL; }

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

    char *name = NULL;
    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter)) {
        name = extract_string_var(&rootIter, &err);
    }
    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
    }

    dbus_message_unref(reply);
    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);
    if (NULL == name) {
        _log (error, "mpris::failed_to_load_player_name");
    } else {
        _log (info, "  loaded::player_name: %s", name);
    }

    return name;

_unref_pending_err:
    {
        dbus_pending_call_unref(pending);
        goto _unref_message_err;
    }
_unref_message_err:
    {
        dbus_message_unref(msg);
    }
    return NULL;
}

#if 0
static char* get_dbus_string_scalar(DBusMessage* message)
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
#endif

char *get_player_namespace(DBusConnection* conn)
{
    if (NULL == conn) { return false; }

    const char* method = DBUS_METHOD_LIST_NAMES;
    const char* destination = DBUS_DESTINATION;
    const char* path = DBUS_PATH;
    const char* interface = DBUS_INTERFACE;
    const char* mpris_namespace = MPRIS_PLAYER_NAMESPACE;
    char* player_namespace = NULL;

    DBusMessage* msg;
    DBusPendingCall* pending;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return player_namespace; }

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
        while (dbus_message_iter_has_next(&arrayElementIter)) {
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayElementIter)) {
                char *value;
                dbus_message_iter_get_basic(&arrayElementIter, &value);
                if (strncmp(value, mpris_namespace, strlen(mpris_namespace)) == 0) {
                    size_t len = strlen(value);
                    player_namespace = get_zero_string(len);
                    strncpy(player_namespace, value, len);
                    break;
                }
            }
            dbus_message_iter_next(&arrayElementIter);
        }
    }
    dbus_message_unref(reply);
    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);
    return player_namespace;

_unref_pending_err:
    {
        dbus_pending_call_unref(pending);
    }
_unref_message_err:
    {
        dbus_message_unref(msg);
    }
    return NULL;
}

static void load_properties(DBusMessageIter *rootIter, mpris_properties *properties)
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
                    properties->can_control = extract_boolean_var(&dictIter, &err);
                    _log (info, "  loaded::can_control: %s", (properties->can_control ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_CANGONEXT, strlen(MPRIS_PNAME_CANGONEXT))) {
                    properties->can_go_next = extract_boolean_var(&dictIter, &err);
                    _log (info, "  loaded::can_go_next: %s", (properties->can_go_next ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_CANGOPREVIOUS, strlen(MPRIS_PNAME_CANGOPREVIOUS))) {
                    properties->can_go_previous = extract_boolean_var(&dictIter, &err);
                    _log (info, "  loaded::can_go_previous: %s", (properties->can_go_previous ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_CANPAUSE, strlen(MPRIS_PNAME_CANPAUSE))) {
                    properties->can_pause = extract_boolean_var(&dictIter, &err);
                    _log (info, "  loaded::can_pause: %s", (properties->can_pause ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_CANPLAY, strlen(MPRIS_PNAME_CANPLAY))) {
                    properties->can_play = extract_boolean_var(&dictIter, &err);
                    _log (info, "  loaded::can_play: %s", (properties->can_play ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_CANSEEK, strlen(MPRIS_PNAME_CANSEEK))) {
                    properties->can_seek = extract_boolean_var(&dictIter,  &err);
                    _log (info, "  loaded::can_seek: %s", (properties->can_seek ? "true" : "false"));
                }
                if (!strncmp(key, MPRIS_PNAME_LOOPSTATUS, strlen(MPRIS_PNAME_LOOPSTATUS))) {
                    properties->loop_status = extract_string_var(&dictIter, &err);
                    _log (info, "  loaded::loop_status: %s", properties->loop_status);
                }
                if (!strncmp(key, MPRIS_PNAME_METADATA, strlen(MPRIS_PNAME_METADATA))) {
                    load_metadata(&dictIter, properties->metadata);
                }
                if (!strncmp(key, MPRIS_PNAME_PLAYBACKSTATUS, strlen(MPRIS_PNAME_PLAYBACKSTATUS))) {
                    properties->playback_status = extract_string_var(&dictIter, &err);
                    _log (info, "  loaded::playback_status: %s", properties->playback_status);
                }
                if (!strncmp(key, MPRIS_PNAME_POSITION, strlen(MPRIS_PNAME_POSITION))) {
                    properties->position = extract_int64_var(&dictIter, &err);
                    _log (info, "  loaded::position: %" PRId64, properties->position);
                }
                if (!strncmp(key, MPRIS_PNAME_SHUFFLE, strlen(MPRIS_PNAME_SHUFFLE))) {
                    properties->shuffle = extract_boolean_var(&dictIter, &err);
                    _log (info, "  loaded::shuffle: %s", (properties->shuffle ? "yes" : "no"));
                }
                if (!strncmp(key, MPRIS_PNAME_VOLUME, strlen(MPRIS_PNAME_VOLUME))) {
                    properties->volume = extract_double_var(&dictIter, &err);
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
#if 0
    } else {
        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(rootIter)) {
            char *key;
            dbus_message_iter_get_basic(rootIter, &key);

            _log (tracing, "\tUncoming key %s", key);
        }
#endif
    }
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
        //mpris_properties_zero(properties);
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

    properties->player_name = get_player_identity(conn, destination);
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

#if 0
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
#endif

static DBusHandlerResult load_properties_from_message(DBusMessage *msg, mpris_properties *data)
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
        while(true) {
            load_properties(&args, data);
            if (!dbus_message_iter_has_next(&args)) {
                break;
            }
            dbus_message_iter_next(&args);
        }
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

static void dispatch(int fd, short ev, void *data)
{
    state *state = data;
    dbus *ctx = state->dbus;
    DBusConnection *conn = ctx->conn;

    _log(tracing,"dbus::dispatching fd=%d, data=%p ev=%d", fd, (void*)data, ev);
    while (dbus_connection_get_dispatch_status(conn) == DBUS_DISPATCH_DATA_REMAINS) {
        dbus_connection_dispatch(conn);
    }
}

static void handle_dispatch_status(DBusConnection *conn, DBusDispatchStatus status, void *data)
{
    state *state = data;

    if (status == DBUS_DISPATCH_DATA_REMAINS) {
        struct timeval tv = { .tv_sec = 0, .tv_usec = 0, };

        event_add (state->events->dispatch, &tv);
        _log(tracing, "dbus::new_dispatch_status(%p): %s", (void*)conn, "DATA_REMAINS");
        _log(tracing, "events::add_event(%p):dispatch", state->events->dispatch);
    }
    if (status == DBUS_DISPATCH_COMPLETE) {
        _log(tracing,"dbus::new_dispatch_status(%p): %s", (void*)conn, "COMPLETE");
    }
    if (status == DBUS_DISPATCH_NEED_MEMORY) {
        _log(tracing,"dbus::new_dispatch_status(%p): %s", (void*)conn, "OUT_OF_MEMORY");
    }
}

static void handle_watch(int fd, short events, void *data)
{
    state *state = data;
    dbus *ctx = state->dbus;

    DBusWatch *watch = ctx->watch;

    unsigned flags = 0;

    if (events & EV_READ) { flags |= DBUS_WATCH_READABLE; }

    _log(tracing,"dbus::handle_event: fd=%d, watch=%p ev=%d", fd, (void*)watch, events);
    if (dbus_watch_handle(watch, flags) == false) {
       _log(error,"dbus::handle_event_failed: fd=%d, watch=%p ev=%d", fd, (void*)watch, events);
    }

    handle_dispatch_status(ctx->conn, DBUS_DISPATCH_DATA_REMAINS, data);
}

static unsigned add_watch(DBusWatch *watch, void *data)
{
    if (!dbus_watch_get_enabled(watch)) { return true;}

    state *state = data;
    state->dbus->watch = watch;

    int fd = dbus_watch_get_unix_fd(watch);
    unsigned flags = dbus_watch_get_flags(watch);

    short cond = EV_PERSIST;
    if (flags & DBUS_WATCH_READABLE) { cond |= EV_READ; }

    struct event *event = event_new(state->events->base, fd, cond, handle_watch, state);

    if (NULL == event) { return false; }
    event_add(event, NULL);
    _log(tracing, "events::add_event(%p):add_watch", event);

    dbus_watch_set_data(watch, event, NULL);

    _log(tracing,"dbus::add_watch: watch=%p data=%p", (void*)watch, data);
    return true;
}

static void remove_watch(DBusWatch *watch, void *data)
{
    if (!dbus_watch_get_enabled(watch)) { return; }

    struct event *event = dbus_watch_get_data(watch);

    _log(tracing,"events::del_event(%p):remove_watch data=%p", (void*)event, data);
    if (NULL != event) { event_free(event); }

    dbus_watch_set_data(watch, NULL, NULL);

    _log(tracing,"dbus::removed_watch: watch=%p data=%p", (void*)watch, data);
}

static void toggle_watch(DBusWatch *watch, void *data)
{
    _log(tracing,"dbus::toggle_watch watch=%p data=%p", (void*)watch, data);

    if (dbus_watch_get_enabled(watch)) {
        add_watch(watch, data);
    } else {
        remove_watch(watch, data);
    }
}

void state_loaded_properties(state *, mpris_properties *);
static DBusHandlerResult add_filter(DBusConnection *conn, DBusMessage *message, void *data)
{
    state *state = data;
    if (dbus_message_is_signal(message, DBUS_PROPERTIES_INTERFACE, MPRIS_SIGNAL_PROPERTIES_CHANGED)) {
        _log(tracing,"dbus::filter: received recognized signal on conn=%p", (void*)conn);

        mpris_properties *properties = mpris_properties_new();
        DBusHandlerResult result = load_properties_from_message(message, properties);
        state_loaded_properties(state, properties);
        mpris_properties_unref(properties);
        return result;
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

static void handle_timeout(int fd, short ev, void *data)
{
    state *state = data;

    DBusTimeout *t = state->dbus->timeout;

    _log(tracing,"dbus::timeout_reached: fd=%d ev=%p events=%d", fd, (void*)t, ev);

    dbus_timeout_handle(t);
}

static unsigned add_timeout(DBusTimeout *t, void *data)
{
    state *state = data;

    if (!dbus_timeout_get_enabled(t)) { return 1; }

    _log(tracing,"dbus::add_timeout: %p data=%p", (void*)t, data);
    struct event *event = event_new(state->events->base, -1, EV_TIMEOUT|EV_PERSIST, handle_timeout, t);
    if (NULL == event) {
        _log(tracing,"dbus::add_timeout: failed");
    }

    int ms = dbus_timeout_get_interval(t);
    struct timeval tv = { .tv_sec = ms / 1000, .tv_usec = (ms % 1000) * 1000, };

    event_add(event, &tv);
    _log(tracing, "events::add_event(%p):add_timeout", event);
    dbus_timeout_set_data(t, event, NULL);

    return 1;
}

static void remove_timeout(DBusTimeout *t, void *data)
{
    struct event *event = dbus_timeout_get_data(t);

    _log(tracing,"dbus::del_timeout: %p data=%p event=%p", (void*)t, data, (void*)event);

    _log(tracing, "events::del_event(%p):remove_timeout", event);
    event_free(event);

    dbus_timeout_set_data(t, NULL, NULL);
}

static void toggle_timeout(DBusTimeout *t, void *data)
{
    _log(tracing,"dbus::toggle_timeout: %p", (void*)t);
    if (dbus_timeout_get_enabled(t)) {
        add_timeout(t, data);
    } else {
        remove_timeout(t, data);
    }
}

void dbus_close(state *state)
{
    if (NULL == state->dbus) { return; }
    if (NULL != state->dbus->conn) {
        _log(tracing, "mem::freeing_dbus_connection(%p)", state->dbus->conn);
        dbus_connection_flush(state->dbus->conn);
        dbus_connection_close(state->dbus->conn);
        dbus_connection_unref(state->dbus->conn);
    }
#if 0
    if (NULL != ctx->properties) {
        mpris_properties_unref(ctx->properties);
    }
#endif
    free(state->dbus);
}

dbus *dbus_connection_init(state *state)
{
    DBusConnection *conn = NULL;
    state->dbus = malloc(sizeof(dbus));
    if (NULL == state->dbus) {
        _log(error, "dbus::failed_to_init_libdbus");
        goto _cleanup;
    }
    DBusError err;
    dbus_error_init(&err);

    conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (NULL == conn) {
        _log(error, "dbus::unable_to_get_on_bus");
        goto _cleanup;
    } else {
        _log(tracing, "mem::inited_dbus_connection(%p)", conn);
    }
    dbus_connection_set_exit_on_disconnect(conn, false);

    state->dbus->conn = conn;

    state->events->dispatch = malloc(sizeof(struct event));
    event_assign(state->events->dispatch, state->events->base, -1, EV_TIMEOUT, dispatch, state);

    if (dbus_connection_set_watch_functions(conn, add_watch, remove_watch, toggle_watch, state, NULL) == false) {
        _log(error, "dbus::add_watch_functions: failed");
        goto _cleanup;
    }

    if (dbus_connection_set_timeout_functions(conn, add_timeout, remove_timeout, toggle_timeout, state, NULL) == false) {
        _log(error, "dbus::add_timeout_functions: failed");
        goto _cleanup;
    }

    if (dbus_connection_add_filter(conn, add_filter, state, NULL) == false) {
        _log(error, "dbus::add_filter: failed");
        goto _cleanup;
    }

    dbus_connection_set_dispatch_status_function(conn, handle_dispatch_status, state, NULL);

    const char* signal_sig = "type='signal',interface='" DBUS_PROPERTIES_INTERFACE "'";
    dbus_bus_add_match(conn, signal_sig, &err);

    if (dbus_error_is_set(&err)) {
        _log(error, "dbus::add_signal: %s", err.message);
        dbus_error_free(&err);
        goto _cleanup;
    }

    return state->dbus;
_cleanup:
    if (dbus_error_is_set(&err)) {
        _log(tracing,"dbus::err: %s", err.message);
        dbus_error_free(&err);
    }
    dbus_close(state);
    return NULL;
}
#endif // SDBUS_H
