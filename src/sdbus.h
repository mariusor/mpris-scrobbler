/**
#include "sstrings.h"
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SDBUS_H
#define MPRIS_SCROBBLER_SDBUS_H

#include <dbus/dbus.h>
#include <event.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#ifdef DEBUG
#define LOCAL_NAME                 "org.mpris.scrobbler-debug"
#else
#define LOCAL_NAME                 "org.mpris.scrobbler"
#endif
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

#define MPRIS_ARG_PLAYER_IDENTITY  "Identity"

#define DBUS_PATH                  "/"
#define DBUS_METHOD_LIST_NAMES     "ListNames"
#define DBUS_METHOD_GET_ALL        "GetAll"
#define DBUS_METHOD_GET            "Get"
#define DBUS_METHOD_GET_ID         "GetId"
#define DBUS_METHOD_PING           "Ping"

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
#define MPRIS_METADATA_GENRE        "xesam:genre"
#define MPRIS_METADATA_YEAR         "year"

#define MPRIS_METADATA_MUSICBRAINZ_TRACK_ID         "xesam:musicBrainzTrackID"
#define MPRIS_METADATA_MUSICBRAINZ_ALBUM_ID         "xesam:musicBrainzAlbumID"
#define MPRIS_METADATA_MUSICBRAINZ_ARTIST_ID        "xesam:musicBrainzArtistID"
#define MPRIS_METADATA_MUSICBRAINZ_ALBUMARTIST_ID   "xesam:musicBrainzAlbumArtistID"

#define DBUS_SIGNAL_PROPERTIES_CHANGED   "PropertiesChanged"
#define DBUS_SIGNAL_NAME_OWNER_CHANGED   "NameOwnerChanged"

// The default timeout leads to hangs when calling
//   certain players which don't seem to reply to MPRIS methods
#define DBUS_CONNECTION_TIMEOUT    100 //ms

static DBusMessage *send_dbus_message(DBusConnection *conn, DBusMessage *msg)
{
    if (NULL == conn) { return NULL; }
    if (NULL == msg) { return NULL; }

    DBusPendingCall *pending = NULL;

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        return NULL;
    }
    if (NULL == pending) {
        return NULL;
    }
    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    // get the reply message
    DBusMessage *reply = dbus_pending_call_steal_reply(pending);

    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message

    return reply;
}

static DBusMessage *call_dbus_method(DBusConnection *conn, char *destination, char *path, char *interface, char *method)
{
    if (NULL == conn) { return NULL; }
    if (NULL == destination) { return NULL; }

    // create a new method call and check for errors
    DBusMessage *msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return NULL; }

    DBusMessage *reply = send_dbus_message(conn, msg);
    dbus_message_unref(msg);
    return reply;
}

static void extract_double_var(DBusMessageIter *iter, double *result, DBusError *error)
{
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_DOUBLE == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, result);
#ifdef DEBUG
        _trace2("  dbus::loaded_basic_double[%p]: %f", result, *result);
#endif
    }
    return;
}

static int extract_string_array_var(DBusMessageIter *iter, char result[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH], DBusError *err)
{
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(err, "iter_should_be_variant", "This message iterator must variant type");
        return 0;
    }

    DBusMessageIter variantIter = {0};
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(err, "sub_iter_should_be_array", "This message iterator must have array type");
        return 0;
    }

    DBusMessageIter arrayIter = {0};
    dbus_message_iter_recurse(&variantIter, &arrayIter);
    if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&arrayIter)) {
        dbus_set_error_const(err, "array_invalid", "The array does not contain strings");
        return 0;
    }

    size_t read_count = 0;
    while (read_count < MAX_PROPERTY_COUNT) {
        DBusBasicValue temp = {0};
        dbus_message_iter_get_basic(&arrayIter, &temp);
        int l = strlen(temp.str);

        if (l == 0 || l >= MAX_PROPERTY_LENGTH) { break; }

        memcpy(result[read_count], temp.str, l);
#ifdef DEBUG
        _trace2("  dbus::loaded_array_of_strings[%4zd//%zd//%p]: %s", l, read_count, result[read_count], result[read_count]);
#endif
        read_count++;
        if (!dbus_message_iter_has_next(&arrayIter)) {
            break;
        }
        dbus_message_iter_next(&arrayIter);
    }
    return read_count;
}

static void extract_string_var(DBusMessageIter *iter, char *result, DBusError *error)
{
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (
        DBUS_TYPE_OBJECT_PATH == dbus_message_iter_get_arg_type(&variantIter) ||
        DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&variantIter)
    ) {
        DBusBasicValue temp = {0};
        dbus_message_iter_get_basic(&variantIter, &temp);
        int l = strlen(temp.str);

        if (l > 0 && l < MAX_PROPERTY_LENGTH) {
            memcpy(result, temp.str, l);
#ifdef DEBUG
            _trace2("  dbus::loaded_basic_string[%zd//%p]: %s", l, result, result);
#endif
        }
    }
}

static void extract_int32_var(DBusMessageIter *iter, int32_t *result, DBusError *error)
{
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, result);
#ifdef DEBUG
        _trace2("  dbus::loaded_basic_int32[%p]: %" PRId32, result, *result);
#endif
    }
    return;
}

static void extract_int64_var(DBusMessageIter *iter, int64_t *result, DBusError *error)
{
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (
        DBUS_TYPE_UINT64 == dbus_message_iter_get_arg_type(&variantIter) ||
        DBUS_TYPE_INT64 == dbus_message_iter_get_arg_type(&variantIter) ||
        DBUS_TYPE_UINT32 == dbus_message_iter_get_arg_type(&variantIter) ||
        DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&variantIter)
    ) {
        dbus_message_iter_get_basic(&variantIter, result);
#ifdef DEBUG
        _trace2("  dbus::loaded_basic_int64[%p]: %" PRId64, result, *result);
#endif
    }
    return;
}

static void extract_boolean_var(DBusMessageIter *iter, bool *result, DBusError *error)
{
    dbus_bool_t res = false;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &res);
#ifdef DEBUG
        _trace2("  dbus::loaded_basic_bool[%p]: %s", result, res ? "true" : "false");
#endif
    }
    *result = (res == 1);
    return;
}

static void load_metadata(DBusMessageIter *iter, struct mpris_metadata *track, struct mpris_event *changes)
{
    if (NULL == track) { return; }
    DBusError err = {0};
    dbus_error_init(&err);

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(&err, "iter_should_be_variant", "This message iterator must have variant type");
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

    while (true) {
        char *key = NULL;
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
                extract_int32_var(&dictIter, (int32_t*)&track->bitrate, &err);
                changes->loaded_state |= mpris_load_metadata_bitrate;
            } else if (!strncmp(key, MPRIS_METADATA_ART_URL, strlen(MPRIS_METADATA_ART_URL))) {
                extract_string_var(&dictIter, track->art_url, &err);
                changes->loaded_state |= mpris_load_metadata_art_url;
            } else if (!strncmp(key, MPRIS_METADATA_LENGTH, strlen(MPRIS_METADATA_LENGTH))) {
                extract_int64_var(&dictIter, (int64_t*)&track->length, &err);
                changes->loaded_state |= mpris_load_metadata_length;
            } else if (!strncmp(key, MPRIS_METADATA_TRACKID, strlen(MPRIS_METADATA_TRACKID))) {
                extract_string_var(&dictIter, track->track_id, &err);
                changes->loaded_state |= mpris_load_metadata_track_id;
            } else if (!strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST))) {
                extract_string_array_var(&dictIter, track->album_artist, &err);
                changes->loaded_state |= mpris_load_metadata_album_artist;
            } else if (!strncmp(key, MPRIS_METADATA_ALBUM, strlen(MPRIS_METADATA_ALBUM)) && strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST)) ) {
                extract_string_var(&dictIter, track->album, &err);
                changes->loaded_state |= mpris_load_metadata_album;
            } else if (!strncmp(key, MPRIS_METADATA_ARTIST, strlen(MPRIS_METADATA_ARTIST))) {
                extract_string_array_var(&dictIter, track->artist, &err);
                changes->loaded_state |= mpris_load_metadata_artist;
            } else if (!strncmp(key, MPRIS_METADATA_COMMENT, strlen(MPRIS_METADATA_COMMENT))) {
                extract_string_array_var(&dictIter, track->comment, &err);
                changes->loaded_state |= mpris_load_metadata_comment;
            } else if (!strncmp(key, MPRIS_METADATA_TITLE, strlen(MPRIS_METADATA_TITLE))) {
                extract_string_var(&dictIter, track->title, &err);
                changes->loaded_state |= mpris_load_metadata_title;
            } else if (!strncmp(key, MPRIS_METADATA_TRACK_NUMBER, strlen(MPRIS_METADATA_TRACK_NUMBER))) {
                extract_int32_var(&dictIter, (int32_t*)&track->track_number, &err);
                changes->loaded_state |= mpris_load_metadata_track_number;
            } else if (!strncmp(key, MPRIS_METADATA_URL, strlen(MPRIS_METADATA_URL))) {
                extract_string_var(&dictIter, track->url, &err);
                changes->loaded_state |= mpris_load_metadata_url;
            } else if (!strncmp(key, MPRIS_METADATA_GENRE, strlen(MPRIS_METADATA_GENRE))) {
                extract_string_array_var(&dictIter, track->genre, &err);
                changes->loaded_state |= mpris_load_metadata_genre;
            } else if (!strncmp(key, MPRIS_METADATA_MUSICBRAINZ_TRACK_ID, strlen(MPRIS_METADATA_MUSICBRAINZ_TRACK_ID))) {
                // check for music brainz tags - players supporting this: Rhythmbox
                extract_string_array_var(&dictIter, track->mb_track_id, &err);
                changes->loaded_state |= mpris_load_metadata_mb_track_id;
            } else if (!strncmp(key, MPRIS_METADATA_MUSICBRAINZ_ALBUM_ID, strlen(MPRIS_METADATA_MUSICBRAINZ_ALBUM_ID))) {
                extract_string_array_var(&dictIter, track->mb_album_id, &err);
                changes->loaded_state |= mpris_load_metadata_mb_album_id;
            } else if (!strncmp(key, MPRIS_METADATA_MUSICBRAINZ_ARTIST_ID, strlen(MPRIS_METADATA_MUSICBRAINZ_ARTIST_ID))) {
                extract_string_array_var(&dictIter, track->mb_artist_id, &err);
                changes->loaded_state |= mpris_load_metadata_mb_artist_id;
            } else if (!strncmp(key, MPRIS_METADATA_MUSICBRAINZ_ALBUMARTIST_ID, strlen(MPRIS_METADATA_MUSICBRAINZ_ALBUMARTIST_ID))) {
                extract_string_array_var(&dictIter, track->mb_album_artist_id, &err);
                changes->loaded_state |= mpris_load_metadata_mb_artist_id;
            }
            changes->track_changed = true;
            if (dbus_error_is_set(&err)) {
                _error("dbus::value_error: %s, %s", key, err.message);
                dbus_error_free(&err);
            }
        }

        if (!dbus_message_iter_has_next(&arrayIter)) {
            break;
        }
        dbus_message_iter_next(&arrayIter);
    }
    if (
        changes->loaded_state & mpris_load_metadata_title &&
        !(changes->loaded_state & mpris_load_metadata_mb_track_id)
    ) {
        changes->loaded_state |= mpris_load_metadata_mb_track_id;
    }
    if (
        changes->loaded_state & mpris_load_metadata_album &&
        !(changes->loaded_state & mpris_load_metadata_mb_album_id)
    ) {
        changes->loaded_state |= mpris_load_metadata_mb_album_id;
    }
    if (
        changes->loaded_state & mpris_load_metadata_artist &&
        !(changes->loaded_state & mpris_load_metadata_mb_artist_id)
    ) {
        changes->loaded_state |= mpris_load_metadata_mb_artist_id;
    }
    if (
        changes->loaded_state & mpris_load_metadata_album_artist &&
        !(changes->loaded_state & mpris_load_metadata_mb_album_artist_id)
    ) {
        changes->loaded_state |= mpris_load_metadata_mb_album_artist_id;
    }
}

static void get_player_identity(DBusConnection *conn, const char *destination, char *identity)
{
    if (NULL == conn) { return; }
    if (NULL == destination) { return; }

    const char *interface = DBUS_INTERFACE_PROPERTIES;
    const char *method = DBUS_METHOD_GET;
    const char *path = MPRIS_PLAYER_PATH;
    const char *arg_interface = MPRIS_PLAYER_NAMESPACE;
    const char *arg_identity = MPRIS_ARG_PLAYER_IDENTITY;

    // create a new method call and check for errors
    DBusMessage *msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return /*NULL*/; }

    DBusMessageIter params;
    // append interface we want to get the property from
    dbus_message_iter_init_append(msg, &params);
    if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, &arg_interface)) {
        goto _unref_message_err;
    }

    dbus_message_iter_init_append(msg, &params);
    if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, &arg_identity)) {
        goto _unref_message_err;
    }

    DBusPendingCall *pending;
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
    DBusMessage *reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) { goto _unref_pending_err; }

    DBusMessageIter rootIter;

    DBusError err = {0};
    dbus_error_init(&err);
    if (dbus_message_iter_init(reply, &rootIter)) {
        extract_string_var(&rootIter, identity, &err);
    }
    if (dbus_error_is_set(&err)) {
        _error("  mpris::failed_to_load_player_name: %s", err.message);
        dbus_error_free(&err);
    } else {
        if (strlen(identity) == 0) {
            _error("mpris::empty_player_name: unable to load");
        } else {
            _trace("mpris::player_name: %s", identity);
        }
    }

    dbus_message_unref(reply);
_unref_pending_err:
    // free the pending message handle
    dbus_pending_call_unref(pending);
_unref_message_err:
    // free message
    dbus_message_unref(msg);
    return;
}

#if 0
static char *get_dbus_string_scalar(DBusMessage *message)
{
    if (NULL == message) { return NULL; }
    char *status = NULL;

    DBusMessageIter rootIter;
    if (dbus_message_iter_init(message, &rootIter) &&
        DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&rootIter)) {

        dbus_message_iter_get_basic(&rootIter, &status);
    }

    return status;
}
#endif

int load_player_namespaces(DBusConnection *conn, struct mpris_player *players, int max_player_count)
{
    if (NULL == conn) { return -1; }

    int count = 0;
    const char *mpris_namespace = MPRIS_PLAYER_NAMESPACE;
    // get the reply message
    DBusMessage *reply = call_dbus_method(conn, DBUS_INTERFACE_DBUS, DBUS_PATH, DBUS_INTERFACE_DBUS, DBUS_METHOD_LIST_NAMES);
    if (NULL != reply) {
        DBusMessageIter rootIter;
        if (dbus_message_iter_init(reply, &rootIter) &&
            DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&rootIter)) {
            DBusMessageIter arrayElementIter;

            dbus_message_iter_recurse(&rootIter, &arrayElementIter);
            while (dbus_message_iter_has_next(&arrayElementIter)) {
                if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayElementIter)) {
                    char *value = NULL;
                    dbus_message_iter_get_basic(&arrayElementIter, &value);
                    if (strncmp(value, mpris_namespace, strlen(mpris_namespace)) == 0) {
                        strncpy(players[count].mpris_name, value, MAX_PROPERTY_LENGTH);
                        count++;
                    }
                }
                dbus_message_iter_next(&arrayElementIter);
            }
        }
    }
    dbus_message_unref(reply);

    // iterate over the namespaces and also load unique bus ids
    for (int i = 0; i < count; i++) {
        struct mpris_player *player = &players[i];
        // create a new method call and check for errors
        DBusMessage *reply = call_dbus_method(conn, player->mpris_name, MPRIS_PLAYER_PATH, DBUS_INTERFACE_PEER, DBUS_METHOD_PING);
        if (NULL != reply) {
            const char *bus_id = dbus_message_get_sender(reply);
            if (NULL != bus_id) {
                memcpy(&player->bus_id, bus_id, strlen(bus_id));
            }
        }
        // free reply
        dbus_message_unref(reply);
    }
    return count;
}

static void print_mpris_properties(const struct mpris_properties *properties, enum log_levels level, const struct mpris_event *changes)
{
    unsigned whats_loaded = changes->loaded_state;
    if (whats_loaded == 0) {
        return;
    }
    _log(level, "dbus::loaded_properties_at: %zu", changes->timestamp);
    if (whats_loaded & mpris_load_property_can_control) {
        _log(level, "   can_control: %s", (properties->can_control ? "true" : "false"));
    }
    if (whats_loaded & mpris_load_property_can_go_next) {
        _log(level, "   can_go_next: %s", (properties->can_go_next ? "true" : "false"));
    }
    if (whats_loaded & mpris_load_property_can_go_previous) {
        _log(level, "   can_go_previous: %s", (properties->can_go_previous ? "true" : "false"));
    }
    if (whats_loaded & mpris_load_property_can_pause) {
        _log(level, "   can_pause: %s", (properties->can_pause ? "true" : "false"));
    }
    if (whats_loaded & mpris_load_property_can_play) {
        _log(level, "   can_play: %s", (properties->can_play ? "true" : "false"));
    }
    if (whats_loaded & mpris_load_property_can_seek) {
        _log(level, "   can_seek: %s", (properties->can_seek ? "true" : "false"));
    }
    if (whats_loaded & mpris_load_property_loop_status && strlen(properties->loop_status) > 0) {
        _log(level, "   loop_status: %s", properties->loop_status);
    }
    if (whats_loaded & mpris_load_property_playback_status && strlen(properties->playback_status) > 0) {
        _log(level, "   playback_status: %s", properties->playback_status);
    }
    if (whats_loaded & mpris_load_property_position) {
        _log(level, "   position: %" PRId64, properties->position);
    }
    if (whats_loaded & mpris_load_property_shuffle) {
        _log(level, "   shuffle: %s", (properties->shuffle ? "yes" : "no"));
    }
    if (whats_loaded & mpris_load_property_volume) {
        _log(level, "   volume: %.2f", properties->volume);
    }
    if (whats_loaded & mpris_load_metadata_bitrate) {
        _log(level, "     metadata::bitrate: %" PRId32, properties->metadata.bitrate);
    }
    if (whats_loaded & mpris_load_metadata_art_url && strlen(properties->metadata.art_url) > 0) {
        _log(level, "     metadata::art_url: %s", properties->metadata.art_url);
    }
    if (whats_loaded & mpris_load_metadata_length) {
        _log(level, "     metadata::length: %" PRId64, properties->metadata.length);
    }
    if (whats_loaded & mpris_load_metadata_track_id && strlen(properties->metadata.track_id) > 0) {
        _log(level, "     metadata::track_id: %s", properties->metadata.track_id);
    }
    if (whats_loaded & mpris_load_metadata_album && strlen(properties->metadata.album) > 0) {
        _log(level, "     metadata::album: %s", properties->metadata.album);
    }
    int cnt = MAX_PROPERTY_COUNT;

    char temp[MAX_PROPERTY_LENGTH*MAX_PROPERTY_COUNT+9] = {0};
    if (whats_loaded & mpris_load_metadata_album_artist && strlen(properties->metadata.album_artist[0]) > 0) {
        array_log_with_label(temp, properties->metadata.album_artist, cnt);
        _log(level, "     metadata::album_artist: %s", temp);
    }
    if (whats_loaded & mpris_load_metadata_artist && strlen(properties->metadata.artist[0]) > 0) {
        array_log_with_label(temp, properties->metadata.artist, cnt); 
        _log(level, "     metadata::artist: %s", temp);
    }
    if (whats_loaded & mpris_load_metadata_comment && strlen(properties->metadata.comment[0]) > 0) {
        array_log_with_label(temp, properties->metadata.comment, cnt);
        _log(level, "     metadata::comment: %s", temp);
    }
    if (whats_loaded & mpris_load_metadata_title && strlen(properties->metadata.title) > 0) {
        _log(level, "     metadata::title: %s", properties->metadata.title);
    }
    if (whats_loaded & mpris_load_metadata_track_number) {
        _log(level, "     metadata::track_number: %2" PRId32, properties->metadata.track_number);
    }
    if (whats_loaded & mpris_load_metadata_url && strlen(properties->metadata.url) > 0) {
        _log(level, "     metadata::url: %s", properties->metadata.url);
    }
    if (whats_loaded & mpris_load_metadata_genre && strlen(properties->metadata.genre[0]) > 0) {
        array_log_with_label(temp, properties->metadata.genre, cnt);
        _log(level, "     metadata::genre: %s", temp);
    }
    if (whats_loaded & mpris_load_metadata_mb_track_id && strlen(properties->metadata.mb_track_id[0]) > 0) {
        array_log_with_label(temp, properties->metadata.mb_track_id, cnt);
        _log(level, "     metadata::musicbrainz::track_id: %s", temp);
    }
    if (whats_loaded & mpris_load_metadata_mb_album_id && strlen(properties->metadata.mb_album_id[0]) > 0) {
        array_log_with_label(temp, properties->metadata.mb_album_id, cnt);
        _log(level, "     metadata::musicbrainz::album_id: %s", temp);
    }
    if (whats_loaded & mpris_load_metadata_mb_artist_id && strlen(properties->metadata.mb_artist_id[0]) > 0) {
        array_log_with_label(temp, properties->metadata.mb_artist_id, cnt);
        _log(level, "     metadata::musicbrainz::artist_id: %s", temp);
    }
    if (whats_loaded & mpris_load_metadata_mb_album_artist_id && strlen(properties->metadata.mb_album_artist_id[0]) > 0) {
        array_log_with_label(temp, properties->metadata.mb_album_artist_id, cnt);
        _log(level, "     metadata::musicbrainz::album_artist_id: %s", temp);
    }
}

static void print_mpris_player(const struct mpris_player *pl, enum log_levels level, bool skip_header)
{
    if (!skip_header) {
        _log(level, "  player[%p]: %s %s", pl, pl->mpris_name, pl->bus_id);
    }
    _log(level, "   name:    %s", pl->name);
    _log(level, "   ignored: %s", pl->ignored ? "yes" : "no");
    _log(level, "   deleted: %s", pl->deleted ? "yes" : "no");
    _log(level << 2U, "   scrobbler: %p", pl->scrobbler);
    struct mpris_event e = { .loaded_state = mpris_load_all, };
    print_mpris_properties(&pl->properties, level, &e);
}

static void print_mpris_players(struct mpris_player *players, int player_count, enum log_levels level)
{
    for (int i = 0; i < player_count; i++) {
        struct mpris_player pl = players[i];
        _log(level, "  player[%d:%d]: %s %s", i, player_count, pl.mpris_name, pl.bus_id);
        print_mpris_player(&pl, level, true);
    }
}

static void load_properties(DBusMessageIter *rootIter, struct mpris_properties *properties, struct mpris_event *changes)
{
    if (NULL == properties) { return; }
    if (NULL == rootIter) { return; }

    DBusError err = {0};
    dbus_error_init(&err);

    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(rootIter)) {
        return;
    }
    if (dbus_message_iter_get_element_count(rootIter) == 0) {
        return;
    }

    DBusMessageIter arrayElementIter;
    dbus_message_iter_recurse(rootIter, &arrayElementIter);

    while (true) {
        if (DBUS_TYPE_DICT_ENTRY != dbus_message_iter_get_arg_type(&arrayElementIter)) {
            dbus_set_error_const(&err, "wrong_iter_type", "The argument should be a dict");
            break;
        }

        DBusMessageIter dictIter;
        dbus_message_iter_recurse(&arrayElementIter, &dictIter);

        if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&dictIter)) {
            dbus_set_error_const(&err, "missing_key", "This message iterator doesn't have a key");
            break;
        }
        char *key = NULL;
        dbus_message_iter_get_basic(&dictIter, &key);

        if (!dbus_message_iter_has_next(&dictIter)) {
            break;
        }
        dbus_message_iter_next(&dictIter);

        if (!strncmp(key, MPRIS_PNAME_CANCONTROL, strlen(MPRIS_PNAME_CANCONTROL))) {
            extract_boolean_var(&dictIter, &properties->can_control, &err);
            changes->loaded_state |= mpris_load_property_can_control;
        } else if (!strncmp(key, MPRIS_PNAME_CANGONEXT, strlen(MPRIS_PNAME_CANGONEXT))) {
            extract_boolean_var(&dictIter, &properties->can_go_next, &err);
            changes->loaded_state |= mpris_load_property_can_go_next;
        } else if (!strncmp(key, MPRIS_PNAME_CANGOPREVIOUS, strlen(MPRIS_PNAME_CANGOPREVIOUS))) {
            extract_boolean_var(&dictIter, &properties->can_go_previous, &err);
            changes->loaded_state |= mpris_load_property_can_go_previous;
        } else if (!strncmp(key, MPRIS_PNAME_CANPAUSE, strlen(MPRIS_PNAME_CANPAUSE))) {
            extract_boolean_var(&dictIter, &properties->can_pause, &err);
            changes->loaded_state |= mpris_load_property_can_pause;
        } else if (!strncmp(key, MPRIS_PNAME_CANPLAY, strlen(MPRIS_PNAME_CANPLAY))) {
            extract_boolean_var(&dictIter, &properties->can_play, &err);
            changes->loaded_state |= mpris_load_property_can_play;
        } else if (!strncmp(key, MPRIS_PNAME_CANSEEK, strlen(MPRIS_PNAME_CANSEEK))) {
            extract_boolean_var(&dictIter, &properties->can_seek, &err);
            changes->loaded_state |= mpris_load_property_can_seek;
        } else if (!strncmp(key, MPRIS_PNAME_LOOPSTATUS, strlen(MPRIS_PNAME_LOOPSTATUS))) {
            extract_string_var(&dictIter, properties->loop_status, &err);
            changes->loaded_state |= mpris_load_property_loop_status;
        } else if (!strncmp(key, MPRIS_PNAME_PLAYBACKSTATUS, strlen(MPRIS_PNAME_PLAYBACKSTATUS))) {
            extract_string_var(&dictIter, properties->playback_status, &err);
            changes->playback_status_changed = true;
            changes->player_state = get_mpris_playback_status(properties);
            changes->loaded_state |= mpris_load_property_playback_status;
        } else if (!strncmp(key, MPRIS_PNAME_POSITION, strlen(MPRIS_PNAME_POSITION))) {
            extract_int64_var(&dictIter, &properties->position, &err);
            changes->position_changed = true;
            changes->loaded_state |= mpris_load_property_position;
        } else if (!strncmp(key, MPRIS_PNAME_SHUFFLE, strlen(MPRIS_PNAME_SHUFFLE))) {
            extract_boolean_var(&dictIter, &properties->shuffle, &err);
            changes->loaded_state |= mpris_load_property_shuffle;
        } else if (!strncmp(key, MPRIS_PNAME_VOLUME, strlen(MPRIS_PNAME_VOLUME))) {
            extract_double_var(&dictIter, &properties->volume, &err);
            changes->volume_changed = true;
            changes->loaded_state |= mpris_load_property_volume;
        } else if (!strncmp(key, MPRIS_PNAME_METADATA, strlen(MPRIS_PNAME_METADATA))) {
            load_metadata(&dictIter, &properties->metadata, changes);
        }
        if (dbus_error_is_set(&err)) {
            _warn("dbus::value_error: %s", err.message);
            dbus_error_free(&err);
        }
        if (!dbus_message_iter_has_next(&arrayElementIter)) {
            break;
        }
        dbus_message_iter_next(&arrayElementIter);
    }
    if (changes->loaded_state != mpris_load_nothing) {
        changes->timestamp = time(0);
    }
    if (dbus_error_is_set(&err)) {
        _warn("dbus::iterator_error: %s", err.message);
        dbus_error_free(&err);
        return;
    }
}

#define _copy_if_changed(a, b, whats_loaded, bitflag) \
    if (whats_loaded & bitflag) { \
        if (!_eq(a, b)) { \
            _cpy(a, b); \
        } else { \
            _neg(whats_loaded, bitflag); \
        } \
    }

static void load_properties_if_changed(struct mpris_properties *oldp, const struct mpris_properties *newp, struct mpris_event *changed)
{
    unsigned whats_loaded = changed->loaded_state;
    _copy_if_changed(oldp->can_control, newp->can_control, whats_loaded, mpris_load_property_can_control);
    _copy_if_changed(oldp->can_go_next, newp->can_go_next, whats_loaded, mpris_load_property_can_go_next);
    _copy_if_changed(oldp->can_go_previous, newp->can_go_previous, whats_loaded, mpris_load_property_can_go_previous);
    _copy_if_changed(oldp->can_control, newp->can_control, whats_loaded, mpris_load_property_can_control);
    _copy_if_changed(oldp->can_play, newp->can_play, whats_loaded, mpris_load_property_can_play);
    _copy_if_changed(oldp->can_seek, newp->can_seek, whats_loaded, mpris_load_property_can_seek);
    _copy_if_changed(oldp->loop_status, newp->loop_status, whats_loaded, mpris_load_property_loop_status);
    _copy_if_changed(oldp->playback_status, newp->playback_status, whats_loaded, mpris_load_property_playback_status);
    _copy_if_changed(oldp->position, newp->position, whats_loaded, mpris_load_property_position);
    _copy_if_changed(oldp->shuffle, newp->shuffle, whats_loaded, mpris_load_property_shuffle);
    _copy_if_changed(oldp->volume, newp->volume, whats_loaded, mpris_load_property_volume);
    _copy_if_changed(oldp->metadata.bitrate, newp->metadata.bitrate, whats_loaded, mpris_load_metadata_bitrate);
    _copy_if_changed(oldp->metadata.art_url, newp->metadata.art_url, whats_loaded, mpris_load_metadata_art_url);
    _copy_if_changed(oldp->metadata.length, newp->metadata.length, whats_loaded, mpris_load_metadata_length);
    _copy_if_changed(oldp->metadata.track_id, newp->metadata.track_id, whats_loaded, mpris_load_metadata_track_id);
    _copy_if_changed(oldp->metadata.album, newp->metadata.album, whats_loaded, mpris_load_metadata_album);
    _copy_if_changed(oldp->metadata.album_artist, newp->metadata.album_artist, whats_loaded, mpris_load_metadata_album_artist);
    _copy_if_changed(oldp->metadata.artist, newp->metadata.artist, whats_loaded, mpris_load_metadata_artist);
    _copy_if_changed(oldp->metadata.comment, newp->metadata.comment, whats_loaded, mpris_load_metadata_comment);
    _copy_if_changed(oldp->metadata.title, newp->metadata.title, whats_loaded, mpris_load_metadata_title);
    _copy_if_changed(oldp->metadata.track_number, newp->metadata.track_number, whats_loaded, mpris_load_metadata_track_number);
    _copy_if_changed(oldp->metadata.url, newp->metadata.url, whats_loaded, mpris_load_metadata_url);
    _copy_if_changed(oldp->metadata.genre, newp->metadata.genre, whats_loaded, mpris_load_metadata_genre);
    _copy_if_changed(oldp->metadata.mb_track_id, newp->metadata.mb_track_id, whats_loaded, mpris_load_metadata_mb_track_id);
    _copy_if_changed(oldp->metadata.mb_album_id, newp->metadata.mb_album_id, whats_loaded, mpris_load_metadata_mb_album_id);
    _copy_if_changed(oldp->metadata.mb_artist_id, newp->metadata.mb_artist_id, whats_loaded, mpris_load_metadata_mb_artist_id);
    _copy_if_changed(oldp->metadata.mb_album_artist_id, newp->metadata.mb_album_artist_id, whats_loaded, mpris_load_metadata_mb_album_artist_id);
    changed->loaded_state = whats_loaded;
}

void load_player_mpris_properties(DBusConnection *conn, struct mpris_player *player)
{
    if (NULL == conn) { return; }
    if (NULL == player) { return; }

    DBusMessage *msg;
    DBusPendingCall *pending;
    DBusMessageIter params;

    const char *interface = DBUS_INTERFACE_PROPERTIES;
    const char *method = DBUS_METHOD_GET_ALL;
    const char *path = MPRIS_PLAYER_PATH;
    const char *arg_interface = MPRIS_PLAYER_INTERFACE;

    const char *identity = player->mpris_name;
    if (strlen(identity)) {
        identity = player->bus_id;
    }
    // create a new method call and check for errors
    msg = dbus_message_new_method_call(identity, path, interface, method);
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

    DBusError err = {0};
    dbus_error_init(&err);

    DBusMessage *reply;

    struct mpris_properties properties = {0};
    struct mpris_event changes = {0};
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) {
        goto _unref_pending_err;
    }
    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter) && DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&rootIter)) {
        load_properties(&rootIter, &properties, &changes);
    }
    if (dbus_error_is_set(&err)) {
        _error("mpris::loading_properties_error: %s", err.message);
        dbus_error_free(&err);
    }

    load_properties_if_changed(&player->properties, &properties, &changes);
    player->changed.loaded_state |= changes.loaded_state;

    dbus_message_unref(reply);
_unref_pending_err:
    // free the pending message handle
    dbus_pending_call_unref(pending);
_unref_message_err:
    // free message
    dbus_message_unref(msg);
}

#if 0
void check_for_player(DBusConnection *conn, char **destination, time_t *last_load_time)
{
    if (NULL == conn) { return; }
    time_t current_time;
    bool valid_incoming_player = mpris_player_is_valid(destination);
    time(&current_time);

    if (difftime(current_time, *last_load_time) < 10 && valid_incoming_player) { return; }

    //get_player_namespace(conn, destination);
    time(last_load_time);

    if (mpris_player_is_valid_name(destination)) {
        if (!valid_incoming_player) { _debug("mpris::found_player: %s", *destination); }
    } else {
        if (valid_incoming_player) { _debug("mpris::lost_player"); }
    }
}
#endif

static int load_player_identity_from_message(DBusMessage *msg, struct mpris_player *player)
{
    if (NULL == msg) {
        _warn("dbus::invalid_signal_message(%p)", msg);
        return false;
    }
    if (NULL == player) {
        _warn("dbus::invalid_player_target(%p)", player);
        return false;
    }
    DBusError err = {0};
    dbus_error_init(&err);

    int loaded = 0;
    char *initial = NULL;
    char *old_name = NULL;
    char *new_name = NULL;
    dbus_message_get_args(msg, &err,
        DBUS_TYPE_STRING, &initial,
        DBUS_TYPE_STRING, &old_name,
        DBUS_TYPE_STRING, &new_name,
        DBUS_TYPE_INVALID);
    if (dbus_error_is_set(&err)) {
        _error("mpris::loading_args: %s", err.message);
        dbus_error_free(&err);
        return 0;
    }

    int len_initial = strlen(initial);
    int len_old = strlen(old_name);
    int len_new = strlen(new_name);
    if (strncmp(initial, MPRIS_PLAYER_NAMESPACE, strlen(MPRIS_PLAYER_NAMESPACE)) == 0) {
        memcpy(player->mpris_name, initial, len_initial);
        if (len_new == 0 && len_old > 0) {
            loaded = -1;
            memcpy(player->bus_id, old_name, len_old);
        }
        if (len_new > 0 && len_old == 0) {
            loaded = 1;
            memcpy(player->bus_id, new_name, len_new);
        }
    }

    return loaded;
}

static bool load_properties_from_message(DBusMessage *msg, struct mpris_properties *data, struct mpris_event *changes, const struct mpris_player players[MAX_PLAYERS], const int players_count)
{
    if (NULL == msg) {
        _warn("dbus::invalid_signal_message(%p)", msg);
        return false;
    }
    char *interface = NULL;
    if (NULL == data) {
        _warn("dbus::invalid_properties_target(%p)", data);
        return false;
    }
    const char *bus_id = dbus_message_get_sender(msg);
    if (NULL != bus_id) {
        memcpy(&changes->sender_bus_id, bus_id, strlen(bus_id));
    }
    for (int i = 0; i < players_count; i++) {
        const struct mpris_player player = players[i];
        if ( !strncmp(player.bus_id, changes->sender_bus_id, sizeof(changes->sender_bus_id)) && player.ignored) {
            _trace("dbus::ignored_player: %s", player.name);
            return false;
        }
    }
    DBusMessageIter args;
    // read the parameters
    const char *signal_name = dbus_message_get_member(msg);
    if (!dbus_message_iter_init(msg, &args)) {
        _warn("dbus::missing_signal_args: %s", signal_name);
    }
    _trace2("dbus::signal(%p): %s:%s:%s.%s", msg, changes->sender_bus_id, dbus_message_get_path(msg), dbus_message_get_interface(msg), signal_name);
    // skip first arg and then load properties from all the remaining ones
    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&args)) {
        dbus_message_iter_get_basic(&args, &interface);
    }
    if (NULL == interface) {
        _warn("dbus::invalid_interface(%p)", interface);
        return false;
    }
    dbus_message_iter_next(&args);

    unsigned temp = changes->loaded_state;
    _trace2("dbus::loading_properties_for: %s", interface);
    if (strncmp(interface, MPRIS_PLAYER_NAMESPACE, strlen(MPRIS_PLAYER_NAMESPACE)) != 0) {
        return false;
    }
    while (true) {
        load_properties(&args, data, changes);
        if (!dbus_message_iter_has_next(&args)) {
            break;
        }
        dbus_message_iter_next(&args);
    }

    return !(changes->loaded_state == temp);
}

static void dispatch(int fd, short ev, void *data)
{
    assert(data);
    DBusConnection *conn = data;

    while (dbus_connection_get_dispatch_status(conn) == DBUS_DISPATCH_DATA_REMAINS) {
        dbus_connection_dispatch(conn);
    }
    if (fd != -1) {
        _trace2("dbus::dispatch: fd=%d, data=%p ev=%d", fd, (void*)data, ev);
    }
}

static void handle_dispatch_status(DBusConnection *conn, DBusDispatchStatus status, void *data)
{
    struct state *s = data;
    if (status == DBUS_DISPATCH_DATA_REMAINS) {
        struct timeval tv = { .tv_sec = 0, .tv_usec = 300000, };
        event_add (&s->events.dispatch, &tv);
        _trace2("dbus::new_dispatch_status(%p): %s", (void*)conn, "DATA_REMAINS");
    }
    if (status == DBUS_DISPATCH_COMPLETE) {
        _trace2("dbus::new_dispatch_status(%p): %s", (void*)conn, "COMPLETE");
    }
    if (status == DBUS_DISPATCH_NEED_MEMORY) {
        _error("dbus::new_dispatch_status(%p): %s", (void*)conn, "OUT_OF_MEMORY");
    }
}

static void handle_watch(int fd, short events, void *data)
{
    assert(data);

    struct state *state = data;
    DBusWatch *watch = state->dbus->watch;

    unsigned flags = 0;
    if (events & EV_READ) { flags |= DBUS_WATCH_READABLE; }

    if (dbus_watch_handle(watch, flags) == false) {
       _error("dbus::handle_event_failed: fd=%d, watch=%p ev=%d", fd, (void*)watch, events);
       return;
    } else {
        _trace2("dbus::handle_event: fd=%d, watch=%p ev=%d", fd, (void*)watch, events);
    }

    handle_dispatch_status(state->dbus->conn, DBUS_DISPATCH_DATA_REMAINS, data);
}

static unsigned add_watch(DBusWatch *watch, void *data)
{
    if (!dbus_watch_get_enabled(watch)) { return true;}

    struct state *state = data;
    state->dbus->watch = watch;

    int fd = dbus_watch_get_unix_fd(watch);
    unsigned flags = dbus_watch_get_flags(watch);

    short cond = EV_PERSIST;
    if (flags & DBUS_WATCH_READABLE) { cond |= EV_READ; }

    evutil_make_socket_nonblocking(fd);
    struct event *event = event_new(state->events.base, fd, cond, handle_watch, state);

    if (NULL == event) { return false; }
    event_add(event, NULL);
    _trace("events::add_event(%p):add_watch", event);

    dbus_watch_set_data(watch, event, NULL);

    _trace2("dbus::add_watch: watch=%p data=%p", (void*)watch, data);
    return true;
}

static void remove_watch(DBusWatch *watch, void *data)
{
    if (!dbus_watch_get_enabled(watch)) { return; }

    struct event *event = dbus_watch_get_data(watch);

    _trace("events::del_event(%p):remove_watch data=%p", (void*)event, data);
    if (NULL != event) { event_free(event); }

    dbus_watch_set_data(watch, NULL, NULL);

    _trace("dbus::removed_watch: watch=%p data=%p", (void*)watch, data);
}

static void toggle_watch(DBusWatch *watch, void *data)
{
    _trace("dbus::toggle_watch watch=%p data=%p", (void*)watch, data);
    if (dbus_watch_get_enabled(watch)) {
        add_watch(watch, data);
    } else {
        remove_watch(watch, data);
    }
}

static int mpris_player_remove(struct mpris_player *players, int player_count, struct mpris_player *player)
{
    if (NULL == players) { return -1; }
    if (NULL == player) { return -1; }
    if (player_count == 0) { return 0; }

    for (int i = 0; i < player_count; i++) {
        struct mpris_player *pl = &players[i];
        if (strncmp(pl->bus_id, player->bus_id, strlen(player->bus_id)) == 0) {
            if (i < player_count-1) {
                // move last player in array to current position
                memcpy(&players[i], &players[player_count-1], sizeof(struct mpris_player));
            }
            // free last player and decrease player count
            mpris_player_free(&players[player_count-1]);
            player_count--;
            break;
        }
    }
    return player_count;
}

static void print_properties_if_changed(struct mpris_properties *oldp, const struct mpris_properties *newp, struct mpris_event *changed, enum log_levels level)
{
#if !DEBUG
    if (level >= log_tracing) { return; }
#else
    if (!level_is(_log_level, level)) { return; }
#endif

    unsigned whats_loaded = changed->loaded_state;
    if (whats_loaded == mpris_load_nothing) { return; }

    bool prop_changed = false;
    if (whats_loaded & mpris_load_property_can_go_next) {
        bool changed = oldp->can_go_next != newp->can_go_next;
        if (changed) {
            _log(level, "  can_go_next changed: %s: '%s' - '%s'", _to_bool(changed), _to_bool(oldp->can_go_next), _to_bool(newp->can_go_next));
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_property_can_go_previous) {
        bool changed = oldp->can_go_previous != newp->can_go_previous;
        if (changed) {
            _log(level, "  can_go_previous changed: %s: '%s' - '%s'", _to_bool(changed), _to_bool(oldp->can_go_previous), _to_bool(newp->can_go_previous));
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_property_can_pause) {
        bool changed = oldp->can_pause != newp->can_pause;
        if (changed) {
            _log(level, "  can_pause changed: %s: '%s' - '%s'", _to_bool(changed), _to_bool(oldp->can_pause), _to_bool(newp->can_pause));
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_property_can_play) {
        bool changed = oldp->can_play != newp->can_play;
        if (changed) {
            _log(level, "  can_play changed: %s: '%s' - '%s'", _to_bool(changed), _to_bool(oldp->can_play), _to_bool(newp->can_play));
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_property_can_seek) {
        bool changed = oldp->can_seek != newp->can_seek;
        if (changed) {
            _log(level, "  can_seek changed: %s: '%s' - '%s'", _to_bool(changed), _to_bool(oldp->can_seek), _to_bool(newp->can_seek));
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_property_loop_status) {
        bool changed = !_eq(oldp->loop_status, newp->loop_status);
        if (changed) {
            _log(level, "  loop_status changed: %s: '%s' - '%s'", _to_bool(changed), oldp->loop_status, newp->loop_status);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_property_playback_status) {
        bool changed = !_eq(oldp->playback_status, newp->playback_status);
        if (changed) {
            _log(level, "  playback_status changed: %s: '%s' - '%s'", _to_bool(changed), oldp->playback_status, newp->playback_status);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_property_position) {
        bool changed = (oldp->position != newp->position);
        if (changed) {
            _log(level, "  position changed: %s: '%" PRId64 "' - '%" PRId64 "'", _to_bool(changed), oldp->position, newp->position);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_property_shuffle) {
        bool changed = !_eq(oldp->shuffle, newp->shuffle);
        if (changed) {
            _log(level, "  shuffle changed: %s: '%s' - '%s'", _to_bool(changed), oldp->shuffle, newp->shuffle);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_property_volume) {
        bool changed = (oldp->volume != newp->volume);
        if (changed) {
            _log(level, "  volume changed: %s: '%.2f' - '%.2f'", _to_bool(changed), oldp->volume, newp->volume);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_bitrate) {
        bool changed = (oldp->metadata.bitrate != newp->metadata.bitrate);
        if (changed) {
            _log(level, "  metadata.bitrate changed: %s: '%" PRId32 "' - '%" PRId32 "'", _to_bool(changed), oldp->metadata.bitrate, newp->metadata.bitrate);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_art_url) {
        bool changed = !_eq(oldp->metadata.art_url, newp->metadata.art_url);
        if (changed) {
            _log(level, "  metadata.art_url changed: %s: '%s' - '%s'", _to_bool(changed), oldp->metadata.art_url, newp->metadata.art_url);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_length) {
        bool changed = (oldp->metadata.length != newp->metadata.length);
        if (changed) {
            _log(level, "  metadata.length changed: %s: '%" PRId64 "' - '%" PRId64 "'", _to_bool(changed), oldp->metadata.length, newp->metadata.length);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_track_id) {
        bool changed = !_eq(oldp->metadata.track_id, newp->metadata.track_id);
        if (changed) {
            _log(level, "  metadata.track_id changed: %s: '%s' - '%s'", _to_bool(changed), oldp->metadata.track_id, newp->metadata.track_id);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_album) {
        bool changed = !_eq(oldp->metadata.album, newp->metadata.album);
        if (changed) {
            _log(level, "  metadata.album changed[%zu]: %s: '%s' - '%s'", sizeof(oldp->metadata.album), _to_bool(changed), oldp->metadata.album, newp->metadata.album);
        }
        prop_changed |= changed;
    }
    int cnt = MAX_PROPERTY_COUNT;
    char temp[MAX_PROPERTY_LENGTH*MAX_PROPERTY_COUNT+9] = {0};
    if (whats_loaded & mpris_load_metadata_album_artist) {
        bool changed = !_eq(oldp->metadata.album_artist, newp->metadata.album_artist);
        if (changed) {
            _log(level, "  metadata.album_artist changed: %s", _to_bool(changed));
            const char t[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH] = {0};
            memcpy((char**)t, oldp->metadata.album_artist, sizeof(t));
            array_log_with_label(temp, t, cnt);
            _log(level, "  from: %s", temp);
            array_log_with_label(temp, newp->metadata.album_artist, cnt);
            _log(level, "    to: %s", temp);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_artist) {
        bool changed = !_eq(oldp->metadata.artist, newp->metadata.artist);
        if (changed) {
            _log(level, "  metadata.artist changed: %s", _to_bool(changed));
            const char t[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH] = {0};
            memcpy((char**)t, oldp->metadata.artist, sizeof(t));
            array_log_with_label(temp, t, cnt);
            _log(level, "  from: %s", temp);
            array_log_with_label(temp, newp->metadata.artist, cnt);
            _log(level, "    to: %s", temp);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_comment) {
        bool changed = !_eq(oldp->metadata.comment, newp->metadata.comment);
        if (changed) {
            _log(level, "  metadata.comment changed: %s", _to_bool(changed));
            const char t[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH] = {0};
            memcpy((char**)t, oldp->metadata.comment, sizeof(t));
            array_log_with_label(temp, t, cnt);
            _log(level, "  from: %s", temp);
            array_log_with_label(temp, newp->metadata.comment, cnt);
            _log(level, "    to: %s", temp);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_title) {
        bool changed = !_eq(oldp->metadata.title, newp->metadata.title);
        if (changed) {
            _log(level, "  metadata.title changed: %s: '%s' - '%s'", _to_bool(changed), oldp->metadata.title, newp->metadata.title);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_track_number) {
        bool changed = (oldp->metadata.track_number != newp->metadata.track_number);
        if (changed) {
            _log(level, "  metadata.track_number changed: %s: '%2" PRId32 "' - '%2" PRId32 "'", _to_bool(changed), oldp->metadata.track_number, newp->metadata.track_number);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_url) {
        bool changed = !_eq(oldp->metadata.url, newp->metadata.url);
        if (changed) {
            _log(level, "  metadata.url changed: %s: '%s' - '%s'", _to_bool(changed), oldp->metadata.url, newp->metadata.url);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_genre) {
        bool changed = !_eq(oldp->metadata.genre, newp->metadata.genre);
        if (changed) {
            _log(level, "  metadata.genre changed: %s", _to_bool(changed));
            const char t[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH] = {0};
            memcpy((char**)t, oldp->metadata.genre, sizeof(t));
            array_log_with_label(temp, t, cnt);
            _log(level, "  from: %s", temp);
            array_log_with_label(temp, newp->metadata.genre, cnt);
            _log(level, "    to: %s", temp);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_mb_track_id) {
        bool changed = !_eq(oldp->metadata.mb_track_id, newp->metadata.mb_track_id);
        if (changed) {
            _log(level, "  metadata.mb_track_id changed: %s", _to_bool(changed));
            const char t[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH] = {0};
            memcpy((char**)t, oldp->metadata.mb_track_id, sizeof(t));
            array_log_with_label(temp, t, cnt);
            _log(level, "  from: %s", temp);
            array_log_with_label(temp, newp->metadata.mb_track_id, cnt);
            _log(level, "    to: %s", temp);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_mb_album_id) {
        bool changed = changed;
        if (changed) {
            _log(level, "  metadata.mb_album_id changed: %s", _to_bool(changed));
            const char t[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH] = {0};
            memcpy((char**)t, oldp->metadata.mb_album_id, sizeof(t));
            array_log_with_label(temp, t, cnt);
            _log(level, "  from: %s", temp);
            array_log_with_label(temp, newp->metadata.mb_album_id, cnt);
            _log(level, "    to: %s", temp);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_mb_artist_id) {
        bool changed = !_eq(oldp->metadata.mb_artist_id, newp->metadata.mb_artist_id);
        if (changed) {
            _log(level, "  metadata.mb_artist_id changed: %s", _to_bool(changed));
            const char t[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH] = {0};
            memcpy((char**)t, oldp->metadata.mb_artist_id, sizeof(t));
            array_log_with_label(temp, t, cnt);
            _log(level, "  from: %s", temp);
            array_log_with_label(temp, newp->metadata.mb_artist_id, cnt);
            _log(level, "    to: %s", temp);
        }
        prop_changed |= changed;
    }
    if (whats_loaded & mpris_load_metadata_mb_album_artist_id) {
        bool changed = !_eq(oldp->metadata.mb_album_artist_id, newp->metadata.mb_album_artist_id);
        if (changed) {
            _log(level, "  metadata.mb_album_artist_id changed: %s", _to_bool(changed));
            const char t[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH] = {0};
            memcpy((char**)t, oldp->metadata.mb_album_artist_id, sizeof(t));
            array_log_with_label(temp, t, cnt);
            _log(level, "  from: %s", temp);
            array_log_with_label(temp, newp->metadata.mb_album_artist_id, cnt);
            _log(level, "    to: %s", temp);
        }
        prop_changed |= changed;
    }
    changed->loaded_state = whats_loaded;
    if (!prop_changed) {
        _log(level, "  nothing %s", "changed");
    }
}

static DBusHandlerResult add_filter(DBusConnection *conn, DBusMessage *message, void *data)
{
    bool handled = false;
    struct state *s = data;
    if (dbus_message_is_signal(message, DBUS_INTERFACE_PROPERTIES, DBUS_SIGNAL_PROPERTIES_CHANGED)) {
        if (strncmp(dbus_message_get_path(message), MPRIS_PLAYER_PATH, strlen(MPRIS_PLAYER_PATH)) == 0) {
            struct mpris_properties properties = {0};
            struct mpris_event changed = {0};
            struct mpris_player *player = NULL;

            bool loaded_something = load_properties_from_message(message, &properties, &changed, s->players, s->player_count);
            if (loaded_something) {
                for (int i = 0; i < s->player_count; i++) {
                    player = &(s->players[i]);
                    if (strncmp(player->bus_id, changed.sender_bus_id, strlen(changed.sender_bus_id))) {
                        continue;
                    }
                    if (player->ignored) {
                        break;
                    }
                    load_properties_if_changed(&player->properties, &properties, &changed);
                    print_properties_if_changed(&player->properties, &properties, &changed, log_tracing);
                    player->changed.loaded_state |= changed.loaded_state;
                    player->changed.timestamp = changed.timestamp;
                    handled = true;
                    break;
                }
                if (!handled) {
                    for (int i = s->player_count; i < MAX_PLAYERS; i++) {
                        // player is not yet in list
                        player = &(s->players[i]);
                        if (strlen(player->bus_id) == 0) {
                            continue;
                        }
                        if (strncmp(player->bus_id, changed.sender_bus_id, strlen(changed.sender_bus_id)) != 0) {
                            continue;
                        }
                        if (player->ignored) {
                            break;
                        }

                        if (mpris_player_init(s->dbus, player, s->events, &s->scrobbler, s->config->ignore_players, s->config->ignore_players_count) > 0) {
                            assert(strlen(player->mpris_name) > 0);
                            _debug("mpris_player::already_opened[%d]: %s%s", i, player->mpris_name, player->bus_id);

                            load_properties_if_changed(&player->properties, &properties, &changed);
                            print_properties_if_changed(&player->properties, &properties, &changed, log_tracing);
                            player->changed.loaded_state |= changed.loaded_state;
                            player->changed.timestamp = changed.timestamp;

                            handled = true;
                            s->player_count++;
                        }
                        break;
                    }
                }
                if (mpris_player_is_valid(player)) {
                    //print_mpris_player(player, log_tracing, false);
                    state_loaded_properties(conn, player, &player->properties, &player->changed);
                }
            } else {
                _warn("mpris_player::unable to load properties from message");
            }
        }
    }
    if (dbus_message_is_signal(message, DBUS_INTERFACE_DBUS, DBUS_SIGNAL_NAME_OWNER_CHANGED)) {
        struct mpris_player temp_player = {0};
        int loaded_or_deleted = load_player_identity_from_message(message, &temp_player);

        handled = loaded_or_deleted != 0;
        if (loaded_or_deleted > 0) {
            // player was opened
            // use the new pointer for inintializing the player and stuff
            struct mpris_player *player = &s->players[s->player_count];
            memcpy(player, &temp_player, sizeof(struct mpris_player));

            mpris_player_init(s->dbus, player, s->events, &s->scrobbler, s->config->ignore_players, s->config->ignore_players_count);
            if (mpris_player_is_valid(player)) {
                //print_mpris_player(player, log_tracing, false);
                state_loaded_properties(conn, player, &player->properties, &player->changed);
            }
            _info("mpris_player::opened[%d]: %s%s", s->player_count, player->mpris_name, player->bus_id);
            s->player_count++;
        } else if (loaded_or_deleted < 0) {
            // player was closed
            s->player_count = mpris_player_remove(s->players, s->player_count, &temp_player);
            _info("mpris_player::closed[%d]: %s%s", s->player_count, temp_player.mpris_name, temp_player.bus_id);
        }
    }
    if (handled) {
        _trace2("dbus::filtered(%p:%p):%s %d %s -> %s %s::%s",
               conn,
               message,
               dbus_message_get_member(message),
               dbus_message_get_type(message),
               dbus_message_get_sender(message),
               dbus_message_get_destination(message),
               dbus_message_get_path(message),
               dbus_message_get_interface(message)
        );
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void dbus_close(struct state *state)
{
    if (NULL == state->dbus) { return; }
    if (NULL != state->dbus->conn) {
        _trace2("mem::free::dbus_connection(%p)", state->dbus->conn);
        dbus_connection_flush(state->dbus->conn);
        dbus_connection_close(state->dbus->conn);
        dbus_connection_unref(state->dbus->conn);
    }
    free(state->dbus);
}

struct dbus *dbus_connection_init(struct state *state)
{
    state->dbus = calloc(1, sizeof(struct dbus));
    if (NULL == state->dbus) {
        _error("dbus::failed_to_init_libdbus");
        goto _cleanup;
    }
    DBusError err = {0};
    dbus_error_init(&err);

    state->dbus->conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (NULL == state->dbus->conn) {
        _error("dbus::unable_to_connect");
        goto _cleanup;
    } else {
        _trace2("mem::inited_dbus_connection(%p)", state->dbus->conn);
    }
    DBusConnection *conn = state->dbus->conn;

    unsigned int flags = DBUS_NAME_FLAG_DO_NOT_QUEUE;
    int name_acquired = dbus_bus_request_name (conn, LOCAL_NAME, flags, &err);
    if (name_acquired == DBUS_REQUEST_NAME_REPLY_EXISTS) {
        _error("dbus::another_instance_running: exiting");
        goto _cleanup;
    }

    event_assign(&state->events.dispatch, state->events.base, -1, EV_TIMEOUT, dispatch, conn);

    const char *properties_match_signal = "type='signal',interface='" DBUS_INTERFACE_PROPERTIES "',member='" DBUS_SIGNAL_PROPERTIES_CHANGED "',path='" MPRIS_PLAYER_PATH "'";
    dbus_bus_add_match(conn, properties_match_signal, &err);
    _trace("dbus::add_match: %s", properties_match_signal);
    if (dbus_error_is_set(&err)) {
        _error("dbus::add_match: %s", err.message);
        dbus_error_free(&err);
        goto _cleanup;
    }
    const char *names_signal = "type='signal',interface='" DBUS_INTERFACE_DBUS "',member='" DBUS_SIGNAL_NAME_OWNER_CHANGED "',path='" DBUS_PATH_DBUS "'";
    dbus_bus_add_match(conn, names_signal, &err);
    _trace("dbus::add_match: %s", names_signal);
    if (dbus_error_is_set(&err)) {
        _error("dbus::add_match: %s", err.message);
        dbus_error_free(&err);
        goto _cleanup;
    }
    // adding dbus filter/watch events
    if (!dbus_connection_add_filter(conn, add_filter, state, NULL)) {
        _error("dbus::add_filter: failed");
        goto _cleanup;
    }

    if (!dbus_connection_set_watch_functions(conn, add_watch, remove_watch, toggle_watch, state, NULL)) {
        _error("dbus::add_watch_functions: failed");
        goto _cleanup;
    }

    dbus_connection_set_dispatch_status_function(conn, handle_dispatch_status, state, NULL);

    dbus_connection_set_exit_on_disconnect(conn, false);
    state->dbus->conn = conn;

    return state->dbus;
_cleanup:
    if (dbus_error_is_set(&err)) {
        _trace("dbus::err: %s", err.message);
        dbus_error_free(&err);
    }
    dbus_close(state);
    return NULL;
}
#endif // MPRIS_SCROBBLER_SDBUS_H
