/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <dbus/dbus.h>
#include <stdlib.h>
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

#define ARG_LASTFM          "lastfm"
#define ARG_LIBREFM         "librefm"

#define HELP_MESSAGE        "MPRIS scrobbler user signon, version %s\n" \
"Usage:\n  %s SERVICE - Open the authorization process for SERVICE\n" \
"Services:\n"\
"\t" ARG_LASTFM "\t\tlast.fm\n" \
"\t" ARG_LIBREFM "\t\tlibre.fm\n" \
HELP_OPTIONS \
""

#define XDG_OPEN "/usr/bin/xdg-open '%s'"

const char* get_version(void)
{
#ifndef VERSION_HASH
#define VERSION_HASH "(unknown)"
#endif
    return VERSION_HASH;
}

enum log_levels _log_level = debug | info | warning | error;
struct configuration global_config = { .name = NULL, .credentials = {NULL, NULL}, .credentials_length = 0};

void print_help(char* name)
{
    const char* help_msg;
    const char* version = get_version();

    help_msg = HELP_MESSAGE;

    fprintf(stdout, help_msg, version, name);
}

void reload_daemon(char *name, char* path)
{
    char *pid_path = get_full_pid_path(name, path);
    FILE *pid_file = fopen(pid_path, "r");
    if (NULL == pid_file) {
        _warn("signon::daemon_reload: unable to find PID file");
        return;
    }

    size_t pid;
    if (fscanf(pid_file, "%lu", &pid) == 1 && kill(pid, SIGHUP) == 0) {
        _debug("signon::daemon_reload[%lu]: ok", pid);
    } else {
        _warn("signon::daemon_reload[%lu]: failed", pid);
    }
}
/**
 * TODO list
 * 1. signon :D
 */
int main (int argc, char** argv)
{
    char* name = argv[0];
    char* command = NULL;
    enum api_type service = unknown;
    enum api_type valid_services[2] = {lastfm, librefm};

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
            _warn("signon::debug: extra verbose output is disabled");
#endif
        }
        if (strncmp(command, ARG_LASTFM, strlen(ARG_LASTFM)) == 0) {
            service = lastfm;
        }
        if (strncmp(command, ARG_LIBREFM, strlen(ARG_LIBREFM)) == 0) {
            service = librefm;
        }
    }
    if(service == unknown) {
        _error("signon::debug: no service selected");
        return EXIT_FAILURE;
    }

    struct configuration *config = malloc(sizeof(struct configuration));
    load_configuration(config, APPLICATION_NAME);
    if (config->credentials_length == 0) {
        _warn("main::load_credentials: no credentials were loaded");
    }
    int opened = -1;
    CURL *curl = curl_easy_init();
    for(size_t i = 0; i < array_count(valid_services); i++) {
        struct api_credentials *cur = api_credentials_new();
        cur->end_point = valid_services[i];
        if (cur->end_point != service) { continue; }

        struct http_request *req = api_build_request_get_token(curl, cur->end_point);
        struct http_response *res = http_response_new();
        // TODO: do something with the response to see if the api call was successful
        enum api_return_statuses ok = api_get_request(curl, req, res);

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

        bool replaced = false;
        if (config->credentials_length > 0) {
            for (size_t j = 0; j < config->credentials_length; j++) {
                struct api_credentials *creds = config->credentials[j];
                if (creds->end_point == cur->end_point) {
                    replaced = true;
                    free(creds);
                    config->credentials[j] = cur;
                }
            }
        }
        if (!replaced) {
            config->credentials[config->credentials_length] = cur;
            config->credentials_length += 1;
        }
        if (NULL == cur->token) { continue; }
        char *auth_url = api_get_auth_url(cur->end_point, cur->token);
        if (NULL == auth_url) { continue; }

        size_t auth_url_len = strlen(auth_url);
        size_t cmd_len = strlen(XDG_OPEN);
        size_t len = cmd_len + auth_url_len;

        char *open_cmd = get_zero_string(len);
        snprintf(open_cmd, len, XDG_OPEN, auth_url);
        _trace("xdg::open: %s", auth_url);

        opened = system(open_cmd);
        free(open_cmd);
    }
    curl_easy_cleanup(curl);


    char *path = get_config_file();
    if (NULL != path) {
        struct ini_config *to_write = get_ini_from_credentials(config->credentials, config->credentials_length);
        _debug("saving::config[%u]: %s", config->credentials_length, path);
        if (write_config(to_write, path) && (opened == 0)) {
            reload_daemon(APPLICATION_NAME, NULL);
        } else {
            _warn("signon::config_error: unable to write to configuration file");
        }
        ini_config_free(to_write);
    }
    free_configuration(config);
    free(config);
    return EXIT_SUCCESS;
}
