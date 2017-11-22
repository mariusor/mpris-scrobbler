/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <curl/curl.h>
#include <event.h>
#include <expat.h>
#include <dbus/dbus.h>
#include <time.h>
#include "version.h"
#include "structs.h"
#include "utils.h"
#include "api.h"
#include "xml.h"
#include "smpris.h"
#include "scrobble.h"
#include "sdbus.h"
#include "sevents.h"
#include "version.h"
#include "ini.h"
#include "configuration.h"

#define HELP_MESSAGE        "MPRIS scrobbler daemon, version %s\n" \
"Usage:\n  %s\t\tstart daemon\n" \
HELP_OPTIONS \
"\t" ARG_PID " PATH\t\tWrite PID to this path\n" \
""


static void print_help(char *name)
{
    const char *help_msg;
    const char *version = get_version();

    help_msg = HELP_MESSAGE;

    fprintf(stdout, help_msg, version, name);
}

/**
 * TODO list
 *  1. Add support to blacklist players (I don't want to submit videos from vlc, for example)
 */
int main (int argc, char *argv[])
{
    // TODO(marius): make this asynchronous to be requested when submitting stuff
    struct parsed_arguments *arguments = parse_command_line(daemon_bin, argc, argv);
    if (arguments->has_help) {
        print_help(arguments->name);
        free_arguments(arguments);
        return EXIT_SUCCESS;
    }
    if (arguments->has_pid && strlen(arguments->pid_path) == 0) {
        _error("main::argument_missing: " ARG_PID " requires a writeable PID path");
        free_arguments(arguments);
        return EXIT_FAILURE;
    }

    struct configuration *config = configuration_new();
    load_configuration(config, APPLICATION_NAME);
    if (config->credentials_length == 0) { _warn("main::load_credentials: no credentials were loaded"); }

    struct state *state = state_new();
    if (NULL == state) { return EXIT_FAILURE; }

    struct sighandler_payload *sig_data = calloc(1, sizeof(struct sighandler_payload));
    if (NULL == sig_data) { return EXIT_FAILURE; }

    sig_data->config = config;
    if (!state_init(state, sig_data)) { return EXIT_FAILURE; }

    char *full_pid_path = get_pid_file(config);
    if (NULL == full_pid_path) { return EXIT_FAILURE; }

    _trace("main::writing_pid: %s", full_pid_path);
    bool wrote_pid = write_pid(full_pid_path);

#if 0
    for(size_t i = 0; i < state->scrobbler->credentials_length; i++) {
        struct api_credentials *cur = state->scrobbler->credentials[i];
        if (NULL == cur) { continue; }
        if (cur->enabled && NULL == cur->session_key && NULL != cur->token) {
            CURL *curl = curl_easy_init();
            struct http_request *req = api_build_request_get_session(curl, cur->end_point, cur->token);
            struct http_response *res = http_response_new();
            // TODO: do something with the response to see if the api call was successful
            enum api_return_statuses ok = api_get_request(curl, req, res);
            //enum api_return_statuses ok = status_ok;

            http_request_free(req);
            if (ok == status_ok) {
                cur->authenticated = true;
                cur->user_name = api_response_get_name(res->doc);
                cur->session_key = api_response_get_key(res->doc);
                _info("api::get_session%s] %s", get_api_type_label(cur->end_point), "ok");
            } else {
                cur->authenticated = false;
                cur->enabled = false;
                _error("api::get_session[%s] %s - disabling", get_api_type_label(cur->end_point), "nok");
            }
            http_response_free(res);
            curl_easy_cleanup(curl);
        }
    }
#endif

    event_base_dispatch(state->events->base);
    state_free(state);
    if (wrote_pid) {
        _debug("main::cleanup_pid: %s", full_pid_path);
        cleanup_pid(full_pid_path);
        free(full_pid_path);
    }
    free_arguments(arguments);
    free_configuration(config);
    free(sig_data);

    return EXIT_SUCCESS;
}
