/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#include <time.h>
#include "structs.h"
#include "utils.h"
#include "smpris.h"
#include "sevents.h"
#include "slastfm.h"
#include "sdbus.h"

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

    state *state = state_new();
    if (NULL == state) { return EXIT_FAILURE; }

    event_base_dispatch(state->events->base);
    state_free(state);

    return EXIT_SUCCESS;
}
