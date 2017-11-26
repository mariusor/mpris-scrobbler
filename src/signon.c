/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#define _POSIX_SOURCE
#include <curl/curl.h>
#include <event.h>
#include <expat.h>
#include <dbus/dbus.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include "version.h"
#include "structs.h"
#include "utils.h"
#include "api.h"
#include "smpris.h"
#include "scrobble.h"
#include "sdbus.h"
#include "sevents.h"
#include "version.h"
#include "ini.h"
#include "configuration.h"

#define HELP_MESSAGE        "MPRIS scrobbler user signon, version %s\n" \
"Usage:\n  %s SERVICE - Open the authorization process for SERVICE\n" \
"Services:\n"\
"\t" ARG_LASTFM "\t\tlast.fm\n" \
"\t" ARG_LIBREFM "\t\tlibre.fm\n" \
HELP_OPTIONS \
""

#define XDG_OPEN "/usr/bin/xdg-open '%s'"

static void print_help(const char *name)
{
    fprintf(stdout, HELP_MESSAGE, get_version(), name);
}

char *get_pid_file(struct configuration *);
static void reload_daemon(struct configuration *config)
{
    char *pid_path = get_pid_file(config);
    FILE *pid_file = fopen(pid_path, "r");
    free(pid_path);
    if (NULL == pid_file) {
        _debug("signon::daemon_reload: unable to find PID file");
        return;
    }

    size_t pid = 0;
    if (fscanf(pid_file, "%lu", &pid) == 1 && kill(pid, SIGHUP) == 0) {
        _info("signon::daemon_reload[%lu]: ok", pid);
    } else {
        _warn("signon::daemon_reload[%lu]: failed", pid);
    }
    fclose(pid_file);
}

static void get_session(struct api_credentials *creds)
{
    if (NULL == creds) { return; }
    if (NULL == creds->token) { return; }

    CURL *curl = curl_easy_init();
    struct http_response *res = http_response_new();
    struct http_request *req = api_build_request_get_session(curl, creds->end_point, creds->token);

    enum api_return_statuses ok = api_get_request(curl, req, res);
    curl_easy_cleanup(curl);
    http_request_free(req);

    if (ok == status_ok && !json_document_is_error(res->body, res->body_length)) {
        api_response_get_session_key_json(res->body, res->body_length, (char**)&creds->session_key, (char**)&creds->user_name);
        if (NULL != creds->session_key) {
            _info("api::get_session[%s] %s", get_api_type_label(creds->end_point), "ok");
        } else {
            _error("api::get_session[%s] %s - disabling", get_api_type_label(creds->end_point), "nok");
            creds->token = NULL;
        }
    }

    http_response_free(res);
}

static void get_token(struct api_credentials *creds)
{
    if (NULL == creds) { return; }
    char *auth_url = NULL;

    CURL *curl = curl_easy_init();
    struct http_response *res = http_response_new();
    struct http_request *req = api_build_request_get_token(curl, creds->end_point);

    enum api_return_statuses ok = api_get_request(curl, req, res);
    curl_easy_cleanup(curl);
    http_request_free(req);

    if (ok == status_ok && !json_document_is_error(res->body, res->body_length)) {
        api_response_get_token_json(res->body, res->body_length, (char**)&creds->token);
    }
    if (NULL != creds->token) {
        _info("api::get_token[%s] %s", get_api_type_label(creds->end_point), "ok");
        auth_url = api_get_auth_url(creds->end_point, creds->token);
        creds->session_key = NULL;
    } else {
        _error("api::get_token[%s] %s - disabling", get_api_type_label(creds->end_point), "nok");
        creds->enabled = false;
    }
    http_response_free(res);

    if (NULL == auth_url) { return; }

    size_t auth_url_len = strlen(auth_url);
    size_t cmd_len = strlen(XDG_OPEN);
    size_t len = cmd_len + auth_url_len;

    char *open_cmd = get_zero_string(len);
    snprintf(open_cmd, len, XDG_OPEN, auth_url);
    bool opened_browser = system(open_cmd);
    if (opened_browser) {
        _debug("xdg::opened[ok]: %s", auth_url);
    } else {
        _debug("xdg::opened[nok]: %s", auth_url);
    }
    free(auth_url);
    free(open_cmd);
}

/**
 * TODO list
 * 1. signon :D
 */
int main (int argc, char *argv[])
{
    struct parsed_arguments *arguments = parse_command_line(signon_bin, argc, argv);

    if (arguments->has_help) {
        print_help(arguments->name);
        free_arguments(arguments);
        return EXIT_SUCCESS;
    }
    if(arguments->service == unknown) {
        _error("signon::debug: no service selected");
        free_arguments(arguments);
        return EXIT_FAILURE;
    }

    bool found = false;
    struct configuration *config = configuration_new();
    load_configuration(config, APPLICATION_NAME);
    if (config->credentials_length == 0) {
        _warn("main::load_credentials: no credentials were loaded");
    }

    struct api_credentials *creds = NULL;
    for(size_t i = 0; i < config->credentials_length; i++) {
        if (config->credentials[i]->end_point != arguments->service) { continue; }

        creds = config->credentials[i];
        found = true;
        break;
    }
    if (NULL == creds) {
        creds = api_credentials_new();
        creds->end_point = arguments->service;
    }
    if (arguments->get_token) {
        _info("signon::getting_token");
        get_token(creds);
    }
    if (arguments->get_session) {
        _info("signon::getting_session_key");
        get_session(creds);
    }
    if (!found) {
        config->credentials[config->credentials_length] = creds;
        config->credentials_length++;
    }
#if 0
    print_application_config(config);
#endif

    if (write_credentials_file(config) == 0) {
        if (arguments->get_session) { reload_daemon(config); }
    } else {
        _warn("signon::config_error: unable to write to configuration file");
    }

    free_configuration(config);
    return EXIT_SUCCESS;
}
