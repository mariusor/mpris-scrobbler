/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
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

    lastfm_scrobbler *scrobbler = lastfm_create_scrobbler(credentials.user_name, credentials.password);
    mpris_properties properties;
    scrobble m;
    scrobble n;

    mpris_properties_init(&properties);
    get_mpris_properties(conn, destination, &properties);
    load_scrobble(&properties, &m);
    bool fresh = true;

    do {
        if (fresh) {
            if (scrobble_is_valid(&n)) {
                lastfm_scrobble(scrobbler, n);
                scrobble_init(&n);
            }
            if (mpris_properties_is_playing(&properties) && now_playing_is_valid(&m)) {
                lastfm_now_playing(scrobbler, m);
                scrobble_copy(&n, &m);
            }
            fresh = false;
            mpris_properties_init(&properties);
        }
        fresh = wait_until_dbus_signal(conn, &properties);
        load_scrobble(&properties, &m);
        usleep(SLEEP_USECS);
    } while (!done);

    dbus_connection_close(conn);
    dbus_connection_unref(conn);

    lastfm_destroy_scrobbler(scrobbler);
    free_credentials(&credentials);
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
