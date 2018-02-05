/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#define _POSIX_SOURCE
#include <curl/curl.h>
#include <dbus/dbus.h>
#include <event.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "structs.h"
#include "utils.h"
#include "api.h"
#include "smpris.h"
#include "scrobble.h"
#include "sdbus.h"
#include "sevents.h"
#include "ini.h"
#include "configuration.h"

#define HELP_MESSAGE        "MPRIS scrobbler user signon, version %s\n\n" \
"Usage:\n %s COMMAND SERVICE - Execute COMMAND for SERVICE\n\n" \
"Commands:\n" \
"\t" ARG_COMMAND_TOKEN "\t\tGet the authentication token for SERVICE.\n" \
"\t" ARG_COMMAND_SESSION "\t\tActivate a new session for SERVICE. SERVICE must have a valid token.\n" \
"\t" ARG_COMMAND_ENABLE "\t\tActivate SERVICE for submitting tracks.\n" \
"\t" ARG_COMMAND_DISABLE "\t\tDeactivate submitting tracks to SERVICE.\n\n" \
"Services:\n" \
"\t" ARG_LASTFM "\t\tlast.fm\n" \
"\t" ARG_LIBREFM "\t\tlibre.fm\n" \
"\t" ARG_LISTENBRAINZ "\tlistenbrainz.org\n\n" \
HELP_OPTIONS \
"\t" ARG_URL_LONG "\tConnect to a custom URL.\n\t\t\t\tValid only for libre.fm and listenbrainz.org \n" \
"\t" ARG_URL "\n" \
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
    struct http_request *req = api_build_request_get_session(curl, creds);

    enum api_return_status ok = api_get_request(curl, req, res);
    curl_easy_cleanup(curl);
    http_request_free(req);

    if (ok == status_ok && !json_document_is_error(res->body, res->body_length, creds->end_point)) {
        api_response_get_session_key_json(res->body, res->body_length, (char**)&creds->session_key, (char**)&creds->user_name, creds->end_point);
        if (NULL != creds->session_key) {
            _info("api::get_session[%s] %s", get_api_type_label(creds->end_point), "ok");
            creds->enabled = true;
        } else {
            _error("api::get_session[%s] %s - disabling", get_api_type_label(creds->end_point), "nok");
            api_credentials_disable(creds);
        }
    } else {
        api_credentials_disable(creds);
    }

    http_response_free(res);
}

static void get_token(struct api_credentials *creds)
{
    if (NULL == creds) { return; }
    char *auth_url = NULL;

    CURL *curl = curl_easy_init();
    struct http_response *res = http_response_new();
    struct http_request *req = api_build_request_get_token(curl, creds);

    enum api_return_status ok = api_get_request(curl, req, res);
    curl_easy_cleanup(curl);
    http_request_free(req);

    if (ok == status_ok && !json_document_is_error(res->body, res->body_length, creds->end_point)) {
        api_credentials_disable(creds);
        api_response_get_token_json(res->body, res->body_length, (char**)&creds->token, creds->end_point);
    }
    if (NULL != creds->token && strlen(creds->token) > 0) {
        _info("api::get_token[%s] %s", get_api_type_label(creds->end_point), "ok");
        creds->enabled = true;
        auth_url = api_get_auth_url(creds->end_point, creds->token);
    } else {
        _error("api::get_token[%s] %s - disabling", get_api_type_label(creds->end_point), "nok");
        api_credentials_disable(creds);
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

static int getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

static void set_token(struct api_credentials *creds)
{
    if (NULL == creds) { return; }

    char chr = 0;
    size_t pos = 0;

    fprintf(stdout, "Token for %s: ", get_api_type_label(creds->end_point));
    while (chr != '\n' && chr != EOF) {
        chr = getch();
        ((char*)creds->token)[pos] = (char)chr;
        pos++;
    }
    ((char*)creds->token)[pos-1] = 0x0;

    if (strlen(creds->token) > 0) {
        _info("api::get_token[%s] %s", get_api_type_label(creds->end_point), "ok");
        creds->enabled = true;
    } else {
        _error("api::get_token[%s] %s - disabling", get_api_type_label(creds->end_point), "nok");
        api_credentials_disable(creds);
    }
}

/**
 * TODO list
 * 1. signon :D
 */
int main (int argc, char *argv[])
{
    int status = EXIT_FAILURE;
    struct parsed_arguments *arguments = NULL;
    arguments = parse_command_line(signon_bin, argc, argv);

    if (arguments->has_help) {
        print_help(arguments->name);
        status = EXIT_SUCCESS;
        goto _free_arguments;
    }
    if(arguments->service == unknown) {
        _error("signon::debug: no service selected");
        status = EXIT_FAILURE;
        goto _free_arguments;
    }

    bool found = false;
    struct configuration *config = configuration_new();
    load_configuration(config, APPLICATION_NAME);
    if (config->credentials_length == 0) {
        _warn("main::load_credentials: no credentials were loaded");
        goto _free_configuration;
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
    if (arguments->disable) {
        _info("signon::disabling: %s", get_api_type_label(arguments->service));
        creds->enabled = false;
    }
    if (arguments->enable) {
        _info("signon::enable: %s", get_api_type_label(arguments->service));
        creds->enabled = true;
    }
    if (arguments->get_token) {
        _info("signon::getting_token: %s", get_api_type_label(arguments->service));

        if (creds->end_point == listenbrainz) {
            set_token(creds);
        } else {
            get_token(creds);
        }
    }
    if (arguments->get_session) {
        _info("signon::getting_session_key: %s", get_api_type_label(arguments->service));
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
        if (arguments->get_session || arguments->disable || arguments->enable) {
            reload_daemon(config);
            status = EXIT_SUCCESS;
        }
    } else {
        _warn("signon::config_error: unable to write to configuration file");
        status = EXIT_FAILURE;
    }

_free_configuration:
    free_configuration(config);
_free_arguments:
    free_arguments(arguments);
    return status;
}
