/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <dbus/dbus.h>
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

#define HELP_MESSAGE        "MPRIS scrobbler daemon, version %s\n" \
"Usage:\n  %s\t\tstart daemon\n" \
HELP_OPTIONS \
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

    for (int i = 0 ; i < argc; i++) {
        command = argv[i];
        if (strcmp(command, ARG_HELP) == 0) {
            print_help(name);
            return EXIT_SUCCESS;
        }
        if (strncmp(command, ARG_QUIET, strlen(ARG_QUIET)) == 0) {
            _log_level = error;
        }
        if (strncmp(command, ARG_VERBOSE1, strlen(ARG_VERBOSE1)) == 0) {
            _log_level = info | warning | error;
        }
        if (strncmp(command, ARG_VERBOSE2, strlen(ARG_VERBOSE2)) == 0) {
            _log_level = debug | info | warning | error;
        }
        if (strncmp(command, ARG_VERBOSE3, strlen(ARG_VERBOSE3)) == 0) {
#ifdef DEBUG
            _log_level = tracing | debug | info | warning | error;
#else
            _warn("main::debug: extra verbose output is disabled");
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

    for(size_t i = 0; i < state->scrobbler->credentials_length; i++) {
        struct api_credentials *cur = state->scrobbler->credentials[i];
        if (NULL == cur) { continue; }
        if (cur->enabled) {
            CURL *curl = curl_easy_init();
            struct http_request *req = api_build_request_get_token(curl, cur->end_point);
            struct http_response *res = http_response_new();
            // TODO: do something with the response to see if the api call was successful
            enum api_return_statuses ok = api_get_request(curl, req, res);
            //enum api_return_statuses ok = status_ok;

            http_request_free(req);
            if (ok == status_ok) {
                cur->authenticated = true;
                cur->token = api_response_get_token(res->doc);
                _info("api::get_token[%s] %s", get_api_type_label(cur->end_point), "ok");
            } else {
                cur->authenticated = false;
                cur->enabled = false;
                _error("api::get_token[%s] %s - disabling", get_api_type_label(cur->end_point), "nok");
            }
            http_response_free(res);
            curl_easy_cleanup(curl);
        }
    }

    event_base_dispatch(state->events->base);
    state_free(state);
    free_configuration(&global_config);

    return EXIT_SUCCESS;
}
