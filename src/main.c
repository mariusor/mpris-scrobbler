/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#include <time.h>
#include "utils.h"
#include "smpris.h"
#include "sdbus.h"
#include "slastfm.h"

log_level _log_level = warning;
struct timeval now_playing_tv;

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

    load_credentials(&credentials);

    struct event_base *eb = event_base_new();
    scrobbles *state = scrobbles_new();
    if (NULL == eb) {
        _log(error, "mem::init_libevent: failure");
        return EXIT_FAILURE;
    } else {
        _log(tracing, "mem::inited_libevent(%p)", eb);
    }
    struct event *sigint_event = evsignal_new(eb, SIGINT, sighandler, eb);
    if (NULL == sigint_event || event_add(sigint_event, NULL) < 0) {
        _log(error, "mem::add_event(SIGINT): failed");
        return EXIT_FAILURE;
    }
    struct event *sigterm_event = evsignal_new(eb, SIGTERM, sighandler, eb);
    if (NULL == sigterm_event || event_add(sigterm_event, NULL) < 0) {
        _log(error, "mem::add_event(SIGTERM): failed");
        return EXIT_FAILURE;
    }
    struct event *sighup_event = evsignal_new(eb, SIGHUP, sighandler, eb);
    if (NULL == sighup_event|| event_add(sighup_event, NULL) < 0) {
        _log(error, "mem::add_event(SIGHUP): failed");
        return EXIT_FAILURE;
    }

    /* Initalize one event */
	event_assign(state->now_playing, eb, -1, EV_PERSIST, now_playing, (void*)&state);

	evutil_timerclear(&now_playing_tv);
    now_playing_tv.tv_sec = 15;
	event_add(state->now_playing, &now_playing_tv);

    evutil_gettimeofday(&lasttime, NULL);

    dbus_ctx* ctx = dbus_connection_init(eb, state->properties);

    event_base_dispatch(eb);

    event_free(sigint_event);
    event_free(sigterm_event);
    event_free(sighup_event);

    if (NULL != ctx) { dbus_close(ctx); }

    return EXIT_SUCCESS;
}
