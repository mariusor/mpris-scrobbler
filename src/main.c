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
        _log(error, "Unable to load credentials");
        goto _error;
    }

    DBusConnection* conn;
    DBusError err;

    // initialise the errors
    dbus_error_init(&err);

    // connect to the system bus and check for errors
    conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection error(%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn) {
        goto _error;
    }

    // request a name on the bus
    int ret = dbus_bus_request_name(conn, LOCAL_NAME,
                               DBUS_NAME_FLAG_REPLACE_EXISTING,
                               &err);
    if (dbus_error_is_set(&err)) {
        //fprintf(stderr, "Name error(%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        goto _dbus_error;
    }

    wait_for_dbus_signal(conn, credentials);
    //now_playing t = {};
    //t.title = "Dionysus";
    //t.artist = "Jocelyn Pook";
    //t.album = "Untold Things";
    //t.length = 300000000;
    //lastfm_now_playing(credentials->user_name, credentials->password, &t);
    free_credentials(credentials);
    dbus_connection_close(conn);
    dbus_connection_unref(conn);

    return EXIT_SUCCESS;

    _dbus_error: {
        dbus_connection_close(conn);
        dbus_connection_unref(conn);

        goto _error;
    }
    _error:
    {
        return EXIT_FAILURE;
    }
}

