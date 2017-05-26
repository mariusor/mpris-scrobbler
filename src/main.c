/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#define MAX_WATCHES 100

#include <time.h>
#include <sys/poll.h>
#include "utils.h"
#include "sdbus.h"
#include "slastfm.h"

bool done = false;
bool reload = true;
log_level _log_level = warning;
struct lastfm_credentials credentials = { NULL, NULL };
struct pollfd pollfds[MAX_WATCHES];
DBusWatch *watches[MAX_WATCHES];
int max_i;

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

    bool valid_credentials = false;
    if (reload) {
        valid_credentials = load_credentials(&credentials);
        reload = false;
    }

    mpris_properties properties = {0};
    mpris_properties_init(&properties);
    init_dbus_connection(&properties);
    valid_credentials = valid_credentials;

#if 0
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

    if (valid_credentials) {
        state.scrobbler = lastfm_create_scrobbler(credentials.user_name, credentials.password);
    }

    time_t current_time, now_playing_time, player_load_time;
    char* destination = (char*)calloc(1, MAX_PROPERTY_LENGTH);

    time(&now_playing_time);
    player_load_time = 0;

    mpris_event last_event = {0};
    mpris_properties properties = {0};
    mpris_properties_init(&properties);
    while (!done) {
        time(&current_time);

// #if 0
        check_for_player(conn, &destination, &player_load_time);
        if (mpris_player_is_valid(&destination)) {
            scrobbles_consume_queue(&state);

            if (fresh) {
                load_event(&properties, &state, &last_event);
                // instantiate a new current track
                mpris_properties current;
                mpris_properties_init(&current);

                get_mpris_properties(conn, destination, &current);
                if (mpris_properties_are_loaded(&current)) {
                    state.playback_status = get_mpris_playback_status(&current);
                    fresh = false;
                }
                if (mpris_properties_is_playing(&current)) {
                    //scrobble_copy(state.current, &current);
                    if (last_event.track_changed) {
                        if (now_playing_is_valid(state.current, current_time, now_playing_time)) {
                            lastfm_now_playing(state.scrobbler, *(state.current));
                            state.current->play_time += difftime(current_time, now_playing_time);
                            time(&now_playing_time);
                        }
                    }
                }
            }
            fresh = wait_until_dbus_signal(conn, state.properties);
            if (fresh) {
                if (NULL != state.current) {
                    state.current->play_time += difftime(current_time, now_playing_time);
                    scrobbles_append(&state, state.current);
                }
            }
// #endif
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
#endif
    return EXIT_SUCCESS;
#if 0
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
#endif
}
