/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#include <time.h>
#include "utils.h"
#include "sdbus.h"
#include "slastfm.h"

bool done = false;
bool reload = true;
log_level _log_level = warning;
struct lastfm_credentials credentials = { NULL, NULL };

int main (int argc, char** argv)
{
    char* command = NULL;
    if (argc > 0) { command = argv[1]; }

    if (NULL != command) {
        if (strncmp(command, "-q", 2) == 0) {
            _log_level = error;
        }
        if (strncmp(command, "-v", 2) == 0) {
            _log_level = info;
        }
        if (strncmp(command, "-vv", 3) == 0) {
            _log_level = debug;
        }
        if (strncmp(command, "-vvv", 4) == 0) {
#ifndef DEBUG
            _log(warning, "main::not_debug_build: tracing output disabled");
#endif
            _log_level = tracing;
        }
    }
    signal(SIGHUP,  sighandler);
    signal(SIGINT,  sighandler);
    signal(SIGTERM, sighandler);

    if (reload) {
        if (!load_credentials(&credentials)) { goto _error; }
        reload = false;
    }

    DBusConnection *conn;

    DBusError err;
    dbus_error_init(&err);

    conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        _log(error, "dbus::connection_error: %s", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn) { goto _error; }

    // request a name on the bus
    int ret = dbus_bus_request_name(conn, LOCAL_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err)) {
        _log(error, "dbus::name_acquire_error: %s", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        _log(error, "dbus::not_alone_on_bus");
        goto _dbus_error;
    }
    bool fresh = true;
    scrobbles state = EMPTY_SCROBBLES;
    scrobbles_init(&state);

    state.scrobbler = lastfm_create_scrobbler(credentials.user_name, credentials.password);
    time_t current_time, now_playing_time, player_load_time;
    char* destination = (char*)calloc(1, MAX_PROPERTY_LENGTH);

    time(&now_playing_time);
    player_load_time = 0;
    while (!done) {
        time(&current_time);

        check_for_player(conn, &destination, &player_load_time);
        scrobbles_consume_queue(&state);

        if (
            scrobbles_is_playing(&state) &&
            now_playing_is_valid(state.current) &&
            (now_playing_time > 0) &&
            (difftime(current_time, now_playing_time) >= LASTFM_NOW_PLAYING_DELAY)
        ) {
            lastfm_now_playing(state.scrobbler, *(state.current));
            state.current->play_time += difftime(current_time, now_playing_time);
            time(&now_playing_time);
        }
        if (fresh) {
            get_mpris_properties(conn, destination, state.properties);
            if (mpris_properties_are_loaded(state.properties)) {
                state.player_state = get_mpris_playback_status(state.properties);
                load_scrobble(state.properties, state.current);
                fresh = false;
            }
            if (scrobbles_is_playing(&state) /* and track was changed */) {
                if (now_playing_is_valid(state.current)) {
                    lastfm_now_playing(state.scrobbler, *(state.current));
                    state.current->play_time += difftime(current_time, now_playing_time);
                    time(&now_playing_time);
                }
            }
        }
        fresh = wait_until_dbus_signal(conn, state.properties);
        if (fresh) {
            if (scrobbles_is_paused(&state) || scrobbles_is_stopped(&state)) {
                now_playing_time = 0;
            }
            if (NULL != state.current) {
                state.current->play_time += difftime(current_time, now_playing_time);
                scrobbles_append(&state, state.current);
            }
        }
        do_sleep(SLEEP_USECS);
    }

    dbus_connection_close(conn);
    dbus_connection_unref(conn);

    // flush the queue
    while(state.queue_length > 0) {
        scrobbles_consume_queue(&state);
    }
    free_credentials(&credentials);
    scrobbles_free(&state);
    free(destination);
    _log(info, "main::exiting...");
    return EXIT_SUCCESS;

    _dbus_error:
    {
        if (dbus_error_is_set(&err)) {
            _log(error, "dbus::error: %s", err.message);
            dbus_error_free(&err);
        }
        dbus_connection_close(conn);
        dbus_connection_unref(conn);

        goto _error;
    }
    _error:
    {
        free_credentials(&credentials);
        _log(error, "main::exiting...");
        return EXIT_FAILURE;
    }
}
