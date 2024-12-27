/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <curl/curl.h>
#include <dbus/dbus.h>
#include <event.h>
#include <time.h>
#ifndef PATH_MAX
// NOTE(marius): musl seems to not have this defined for all cases, Alpine release build
// would fail in CI
#define PATH_MAX 4096
#endif
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include "sstrings.h"
#include "structs.h"
#include "utils.h"
#include "api.h"
#include "smpris.h"
#include "scrobbler.h"
#include "scrobble.h"
#include "sdbus.h"
#include "sevents.h"
#include "configuration.h"

#define HELP_MESSAGE        "MPRIS scrobbler daemon, version %s\n" \
"Usage:\n  %s\t\tStart daemon\n" \
HELP_OPTIONS \
""


static void print_help(const char *name)
{
    fprintf(stdout, HELP_MESSAGE, get_version(), name);
}

int main (const int argc, char *argv[])
{
    int status = EXIT_FAILURE;
    struct configuration config = {0};

    // TODO(marius): make this asynchronous to be requested when submitting stuff
    struct parsed_arguments arguments = {0};
    parse_command_line(&arguments, daemon_bin, argc, argv);
    if (arguments.has_help) {
        print_help(arguments.name);
        status = EXIT_SUCCESS;
        goto _free_arguments;
    }

    load_configuration(&config, APPLICATION_NAME);
    load_pid_path(&config);
    _trace("main::writing_pid: %s", config.pid_path);
    config.wrote_pid = write_pid(config.pid_path);

    const size_t count = arrlen(config.credentials);
    if (count == 0) { _warn("main::load_credentials: no credentials were loaded"); }

    struct state state = {0};

    if (!state_init(&state, &config)) {
        _error("main::unable to initialize");
        goto _free_state;
    }

    event_base_dispatch(state.events.base);
    status = EXIT_SUCCESS;

_free_state:
    state_destroy(&state);
    configuration_clean(&config);
_free_arguments:
    arguments_clean(&arguments);

    return status;
}
