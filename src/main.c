/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#include "utils.h"
#include "slastfm.h"
#include "sdbus.h"

void sighandler(int signum)
{
	_log(info, "Received signal: %d\n", signum);
	exit(EXIT_SUCCESS);
}

int main (int argc, char** argv)
{
    char* command = 0;
    if (argc > 0) {
        command = argv[1];
    }
    if (command == 0) {
    }

    signal(SIGINT,  sighandler);
    signal(SIGTERM, sighandler);

    lastfm_credentials *credentials = load_credentials();
    if (!credentials) {
        _log(error, "last.fm::load_credentials: failed");
        goto _error;
    }

    DBusConnection* conn;
    DBusError err;

    // initialise the errors
    dbus_error_init(&err);

    // connect to the system bus and check for errors
    conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        _log(error, "dbus::connection_error: %s", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn) {
        goto _error;
    }

    // request a name on the bus
    int ret = dbus_bus_request_name(conn, LOCAL_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

    if (dbus_error_is_set(&err)) {
        _log(error, "dbus::name_aquire_error: %s", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        goto _dbus_error;
    }

    add_dbus_signal_wait(conn, &err);
    if (dbus_error_is_set(&err)) {
        _log(error, "dbus::signal_wait_error: %s", err.message);
        dbus_error_free(&err);
    }
    mpris_properties p;
//    bool status_playing = false;
    while (true) {
        if (consume_signal_queue(conn, &p)) {
            if (mpris_properties_is_set(&p)) {
                if (mpris_properties_playback_status_is_set(&p)) {
                    _log(info, "dbus::playback_status_changed: %s", p.playback_status);
//                    if (strncmp(MPRIS_PLAYBACK_STATUS_PLAYING, p.playback_status, strlen(MPRIS_PLAYBACK_STATUS_PLAYING)) == 0) {
//                        status_playing = true;
//                    } else {
//                        status_playing = false;
//                    }
                }
                now_playing n;
                if (mpris_metadata_is_set(&p.metadata)) {
                    n.title = p.metadata.title;
                    n.album = p.metadata.album;
                    n.artist = p.metadata.artist;
                    n.length = p.metadata.length;
                    n.track_number = p.metadata.track_number;
                    _log(info, "last.fm::track_changed %s - %s - %s", n.title, n.artist, n.album);

                    lastfm_now_playing(credentials->user_name, credentials->password, &n);
                }
            }
        } else {
            // loop again if we haven't read a message
            sleep(1);
        }
        //if (p) { mpris_properties_unref(&p); }
    }

    free_credentials(credentials);
    dbus_error_free(&err);
    dbus_connection_close(conn);
    dbus_connection_unref(conn);

    return EXIT_SUCCESS;

    _dbus_error: {
        dbus_error_free(&err);
        dbus_connection_close(conn);
        dbus_connection_unref(conn);

        goto _error;
    }
    _error:
    {
        return EXIT_FAILURE;
    }
}

