/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#define MAX_WATCHES 3

#include <time.h>
#include "utils.h"
#include "sdbus.h"
#include "slastfm.h"

bool valid_credentials = false;
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

    extern bool valid_credentials;
    extern bool reload;
    if (reload) {
        valid_credentials = load_credentials(&credentials);
    }

    struct event_base *eb = event_base_new();
    if (NULL == eb) {
        _log(error, "main::libevent_init_failure");
        return EXIT_FAILURE;
    }
    struct event *sigint_event = evsignal_new(eb, SIGINT, sighandler, eb);
    if (NULL == sigint_event || event_add(sigint_event, NULL) < 0) {
        _log(error, "main::error_adding_sigint_event");
        return EXIT_FAILURE;
    }
    struct event *sigterm_event = evsignal_new(eb, SIGTERM, sighandler, eb);
    if (NULL == sigterm_event || event_add(sigterm_event, NULL) < 0) {
        _log(error, "main::error_adding_sigterm_event");
        return EXIT_FAILURE;
    }
    struct event *sighup_event = evsignal_new(eb, SIGHUP, sighandler, eb);
    if (NULL == sighup_event|| event_add(sighup_event, NULL) < 0) {
        _log(error, "main::error_adding_sighup_event");
        return EXIT_FAILURE;
    }

    mpris_properties *properties = mpris_properties_new();

    dbus_ctx* ctx = dbus_connection_init(eb, properties);

    event_base_dispatch(eb);

    event_free(sigint_event);
    event_free(sigterm_event);
    event_free(sighup_event);

    if (NULL != ctx) { dbus_close(ctx); }

    return EXIT_SUCCESS;
}
