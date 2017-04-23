/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#include "utils.h"
#include "sdbus.h"
#include "slastfm.h"

struct lastfm_credentials credentials;
bool done = false;
bool reload = true;
log_level _log_level = warning;

bool is_playing(const mpris_properties p)
{
    return (NULL != p.playback_status &&
            strncmp(p.playback_status, "Playing", 8) == 0);
}

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
    }
    signal(SIGHUP,  sighandler);
    signal(SIGINT,  sighandler);
    signal(SIGTERM, sighandler);

    if (reload) {
        reload = false;
        if (!load_credentials(&credentials)) { goto _error; }
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

    time_t rawtime = time(0);

    mpris_properties properties;
    mpris_properties_init(&properties);
    bool received = false;
    bool scrobbled = false;
    while (!done) {
        wait_for_dbus_signal(conn, &properties, &received);

        if (received) {
            now_playing m = load_now_playing(&properties);
            if (now_playing_is_valid(&m) && is_playing(properties)) {
                lastfm_now_playing(credentials.user_name, credentials.password, &m);
                rawtime = time(0);
                received = false;
                scrobbled = false;
            }
        }
        usleep(5000);
        if (!scrobbled) {
            scrobble s = load_scrobble(&properties, rawtime);
            if (scrobble_is_valid(&s) && is_playing(properties)) {
                lastfm_scrobble(credentials.user_name, credentials.password, &s);
                scrobbled = true;
            }
        }
    }

    dbus_connection_close(conn);
    dbus_connection_unref(conn);

    free_credentials(&credentials);
    _log(info, "base::exiting...");
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
        _log(error, "base::exiting...");
        return EXIT_FAILURE;
    }
}

