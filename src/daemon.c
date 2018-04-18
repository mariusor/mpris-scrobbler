/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <curl/curl.h>
#include <dbus/dbus.h>
#include <event.h>
#include <time.h>
#include "structs.h"
#include "utils.h"
#include "stretchy_buffer.h"
#include "api.h"
#include "smpris.h"
#include "scrobble.h"
#include "sdbus.h"
#include "sevents.h"
#include "ini.h"
#include "configuration.h"

#define HELP_MESSAGE        "MPRIS scrobbler daemon, version %s\n" \
"Usage:\n  %s\t\tStart daemon\n" \
HELP_OPTIONS \
""


static void print_help(const char *name)
{
    fprintf(stdout, HELP_MESSAGE, get_version(), name);
}

/**
 * TODO list
 *  1. Add support to blacklist players (I don't want to submit videos from vlc, for example)
 */
int main (int argc, char *argv[])
{
    int status = EXIT_FAILURE;
    struct configuration *config = NULL;

    bool wrote_pid = false;
    // TODO(marius): make this asynchronous to be requested when submitting stuff
    struct parsed_arguments *arguments = parse_command_line(daemon_bin, argc, argv);
    if (arguments->has_help) {
        print_help(arguments->name);
        status = EXIT_SUCCESS;
        goto _free_arguments;
    }

    config = configuration_new();
    load_configuration(config, APPLICATION_NAME);
    if (config->credentials_length == 0) { _warn("main::load_credentials: no credentials were loaded"); }
#if 0
    print_application_config(config);
#endif

    struct state *state = state_new();
    if (NULL == state) {
        goto _free_state;
    }

    struct sighandler_payload *sig_data = calloc(1, sizeof(struct sighandler_payload));
    if (NULL == sig_data) {
        goto _free_sig_data;
    }

    sig_data->config = config;
    if (!state_init(state, sig_data)) {
        goto _free_sig_data;
    }

    char *full_pid_path = get_pid_file(config);
    if (NULL == full_pid_path) {
        goto _exit;
    }

    _trace("main::writing_pid: %s", full_pid_path);
    wrote_pid = write_pid(full_pid_path);

    event_base_dispatch(state->events->base);
    status = EXIT_SUCCESS;

_exit:
    if (wrote_pid) {
        _trace("main::cleanup_pid: %s", full_pid_path);
        cleanup_pid(full_pid_path);
        free(full_pid_path);
    }
_free_sig_data:
    if (NULL != sig_data) {
        free(sig_data);
    }
_free_state:
    if (NULL != state) {
        state_free(state);
    }
    free_configuration(config);
_free_arguments:
    free_arguments(arguments);

    return status;
}
