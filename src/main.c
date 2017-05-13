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

    const char* destination = get_player_namespace(conn);
    if (NULL == destination ) { goto _dbus_error; }
    if (strlen(destination) == 0) { goto _dbus_error; }

    bool fresh = true;

    scrobbles state = EMPTY_SCROBBLES;
    scrobbles_init(&state);

    state.scrobbler = lastfm_create_scrobbler(credentials.user_name, credentials.password);

    mpris_properties properties;
    mpris_properties_init(&properties);
    get_mpris_properties(conn, destination, &properties);

    time_t current_time;
    time_t last_time;
    time(&last_time);
    scrobble current;
    do {
        time(&current_time);
        while(state.queue_length > 1) {
            scrobbles_consume_queue(&state);
        }
        if (fresh) {
            state.player_state = get_mpris_playback_status(&properties);
            _log(tracing, "fresh: %s, elapsed %d", fresh ? "true" : "false", difftime(current_time, state.current->start_time));
            load_scrobble(&properties, &current);
            if (scrobbles_has_track_changed(&state, &current)) {
                scrobble_copy(state.current, &current);
                scrobbles_append(&state, state.current);
            }
            if (
                (scrobbles_is_playing(&state) && now_playing_is_valid(state.current)) || 
                (difftime(current_time, last_time) > LASTFM_NOW_PLAYING_DELAY)
            ) {
                lastfm_now_playing(state.scrobbler, *(state.current));
            }
            mpris_properties_init(&properties);
            fresh = false;
        }
        fresh = wait_until_dbus_signal(conn, &properties);
        usleep(SLEEP_USECS);
    } while (!done);

    dbus_connection_close(conn);
    dbus_connection_unref(conn);

    // consume the queue
    while(state.queue_length > 1) {
        scrobbles_consume_queue(&state);
    }
    free_credentials(&credentials);
    scrobbles_free(&state);
    _log(info, "main::exiting...");
    return EXIT_SUCCESS;

    _dbus_error:
    {
        if (dbus_error_is_set(&err)) {
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
