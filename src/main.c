/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <time.h>
#include "structs.h"
#include "ini.c"
#include "utils.h"
#include "api.h"
#include "smpris.h"
#include "scrobble.h"
#include "sdbus.h"
#include "sevents.h"
#include "version.h"

#define ARG_HELP            "-h"
#define ARG_QUIET           "-q"
#define ARG_VERBOSE1        "-v"
#define ARG_VERBOSE2        "-vv"
#define ARG_VERBOSE3        "-vvv"
#define HELP_MESSAGE        "MPRIS scrobbler daemon, version %s\n" \
"Usage:\n  %s\t\tstart daemon\n" \
""

const char* get_version(void)
{
#ifndef VERSION_HASH
#define VERSION_HASH "(unknown)"
#endif
    return VERSION_HASH;
}

enum log_levels _log_level = debug;
struct configuration global_config = { .name = NULL, .credentials = {NULL, NULL}, .credentials_length = 0};

void print_help(char* name)
{
    const char* help_msg;
    const char* version = get_version();

    help_msg = HELP_MESSAGE;

    fprintf(stdout, help_msg, version, name);
}

/**
 * TODO list
 *  1. Build our own last.fm API functionality
 *  2. Add support for libre.fm in the API
 *  3. Add support for credentials on multiple accounts
 *  4. Add support to blacklist players (I don't want to submit videos from vlc, for example)
 */
int main (int argc, char** argv)
{
    char* name = argv[0];
    char* command = NULL;
    if (argc > 0) { command = argv[1]; }

    if (NULL != command) {
        char *command = argv[1];
        if (strcmp(command, ARG_HELP) == 0) {
            print_help(name);
            return EXIT_SUCCESS;
        }
        if (strncmp(command, ARG_QUIET, strlen(ARG_QUIET)) == 0) {
            _log_level = error;
        }
        if (strncmp(command, ARG_VERBOSE1, strlen(ARG_VERBOSE1)) == 0) {
            _log_level = info;
        }
        if (strncmp(command, ARG_VERBOSE2, strlen(ARG_VERBOSE2)) == 0) {
            _log_level = debug;
        }
        if (strncmp(command, ARG_VERBOSE3, strlen(ARG_VERBOSE3)) == 0) {
#ifndef DEBUG
            _warn("main::debug: tracing output is disabled");
            _log_level = info;
#else
            _log_level = tracing;
#endif
        }
    }
    // TODO(marius): make this asynchronous to be requested when submitting stuff
    load_configuration(&global_config);
    if (global_config.credentials_length == 0) {
        _warn("main::load_credentials: no credentials were loaded");
    }

    struct state *state = state_new();
    if (NULL == state) { return EXIT_FAILURE; }

    event_base_dispatch(state->events->base);
    state_free(state);
    free_configuration(&global_config);

    return EXIT_SUCCESS;
}
