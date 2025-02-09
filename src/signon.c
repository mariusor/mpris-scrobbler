/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <curl/curl.h>
#include <dbus/dbus.h>
#include <event.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include "structs.h"
#include "sstrings.h"
#include "utils.h"
#include "api.h"
#include "smpris.h"
#include "scrobbler.h"
#include "scrobble.h"
#include "sdbus.h"
#include "sevents.h"
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

#define XDG_OPEN "/usr/bin/xdg-open %s"

static void print_help(const char *name)
{
    fprintf(stdout, HELP_MESSAGE, get_version(), name);
}

char *get_pid_file(struct configuration *);
static void reload_daemon(struct configuration *config)
{
    load_pid_path(config);
    FILE *pid_file = fopen(config->pid_path, "r");
    if (NULL == pid_file) {
        _debug("signon::daemon_reload: unable to find PID file");
        return;
    }

    pid_t pid = 0;
    if (fscanf(pid_file, "%du", &pid) == 1) {
        if (kill(pid, SIGHUP) == 0) {
            _info("signon::daemon_reload[%zu]: ok", pid);
        } else {
            _warn("signon::daemon_reload[%zu]: failed", pid);
        }
    }
    fclose(pid_file);
}

static enum api_return_status request_call(struct scrobbler_connection *conn)
{
    enum api_return_status ok = status_failed;
    CURLcode cres = curl_easy_perform(conn->handle);
    /* Check for errors */
    if(cres != CURLE_OK) {
        _error("curl::error: %s", curl_easy_strerror(cres));
    }
    curl_easy_getinfo(conn->handle, CURLINFO_RESPONSE_CODE, &conn->response->code);
    _trace("curl::response(%p:%lu): %s", conn->response, conn->response->body_length, conn->response->body);

    if (conn->response->code == 200) { ok = status_ok; }
    return ok;
}

static void get_session(struct api_credentials *creds)
{
    if (NULL == creds) { return; }
    if (strlen(creds->api_key) == 0) {
        _error("invalid_api_key %s %s %s: %s", LASTFM_API_KEY, LIBREFM_API_KEY, LISTENBRAINZ_API_KEY, creds->api_key);
        //return;
    }
    if (strlen(creds->secret)  == 0) {
        _error("invalid_api_secret %s %s %s: %s", LASTFM_API_SECRET, LIBREFM_API_SECRET, LISTENBRAINZ_API_SECRET, creds->secret);
        return;
    }
    if (strlen(creds->token) == 0) { return; }

    struct scrobbler_connection *conn = scrobbler_connection_new();
    scrobbler_connection_init(conn, NULL, *creds, 0);
    conn->request = api_build_request_get_session(creds, conn->handle);

    build_curl_request(conn);

    enum api_return_status ok = request_call(conn);
    if (ok == status_ok && !json_document_is_error(conn->response->body, conn->response->body_length, creds->end_point)) {
        api_response_get_session_key_json(conn->response->body, conn->response->body_length, creds);
        if (strlen(creds->session_key) > 0) {
            _info("api::get_session[%s] %s", get_api_type_label(creds->end_point), "ok");
            creds->enabled = true;
        } else {
            _error("api::get_session[%s] %s - disabling", get_api_type_label(creds->end_point), "nok");
            api_credentials_disable(creds);
        }
    } else {
        api_credentials_disable(creds);
    }
    scrobbler_connection_free(conn);
}

static bool get_token(struct api_credentials *creds)
{
    if (NULL == creds) { return false; }
    if (strlen(creds->api_key) == 0) {
        _error("api::invalid_key");
        return false;
    }
    if (strlen(creds->secret) == 0) {
        _error("api::invalid_secret");
        return false;
    }

    char *auth_url = NULL;

    struct scrobbler_connection *conn = scrobbler_connection_new();
    scrobbler_connection_init(conn, NULL, *creds, 0);
    conn->request = api_build_request_get_token(creds, conn->handle);
    if (NULL == conn->request) {
        _error("api::invalid_get_token_request");
    }

    build_curl_request(conn);

    enum api_return_status ok = request_call(conn);

    if (ok == status_ok && !json_document_is_error(conn->response->body, conn->response->body_length, creds->end_point)) {
        api_credentials_disable(creds);
        api_response_get_token_json(conn->response->body, conn->response->body_length, creds);
    }
    if (strlen(creds->token) > 0) {
        _info("api::get_token[%s] %s", get_api_type_label(creds->end_point), "ok");
        creds->enabled = true;
        auth_url = api_get_auth_url(creds);
    } else {
        _error("api::get_token[%s] %s - disabling", get_api_type_label(creds->end_point), "nok");
        api_credentials_disable(creds);
    }
    scrobbler_connection_free(conn);

    if (NULL == auth_url) {
        _error("signon::get_token_error: unable to open authentication url");
        return false;
    }

    const size_t auth_url_len = strlen(auth_url);
    const size_t cmd_len = strlen(XDG_OPEN);
    const size_t len = cmd_len + auth_url_len;

    char *open_cmd = get_zero_string(len);
    snprintf(open_cmd, len, XDG_OPEN, auth_url);
    const int status = system(open_cmd);

    if (status == EXIT_SUCCESS) {
        _debug("xdg::opened[ok]: %s", auth_url);
    } else {
        _debug("xdg::opened[nok]: %s", auth_url);
    }

    string_free(auth_url);
    string_free(open_cmd);

    return status == EXIT_SUCCESS;
}

static int getch(void) {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~((tcflag_t)ICANON | (tcflag_t)ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    const int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

static void set_token(struct api_credentials *creds)
{
    if (NULL == creds) { return; }

    int chr = 0;
    size_t pos = 0;

    fprintf(stdout, "Token for %s: ", get_api_type_label(creds->end_point));
    while (chr != '\n') {
        chr = getch();
        ((char*)creds->token)[pos] = (char)chr;
        pos++;
    }
    ((char*)creds->token)[pos-1] = 0x0;
    fprintf(stdout, "\n");

    if (strlen(creds->token) > 0) {
        _info("api::get_token[%s] %s", get_api_type_label(creds->end_point), "ok");
        creds->enabled = true;
    } else {
        _error("api::get_token[%s] %s - disabling", get_api_type_label(creds->end_point), "nok");
        api_credentials_disable(creds);
    }
}

int main (const int argc, char *argv[])
{
    int status = EXIT_FAILURE;
    struct parsed_arguments arguments = {0};
    parse_command_line(&arguments, signon_bin, argc, argv);

    if (arguments.has_help) {
        print_help(arguments.name);
        status = EXIT_SUCCESS;
        goto _exit;
    }
    if(arguments.service == api_unknown) {
        _error("signon::debug: no service selected");
        status = EXIT_FAILURE;
        goto _exit;
    }

    bool found = false;
    struct configuration config = {0};
    load_configuration(&config, APPLICATION_NAME);

    const size_t count = arrlen(config.credentials);
    if (count == 0) {
        _warn("main::load_credentials: no credentials were loaded");
    }

    struct api_credentials *creds = NULL;
    for (size_t i = 0; i < count; i++) {
        if (config.credentials[i]->end_point != arguments.service) { continue; }

        creds = config.credentials[i];
        found = true;
        break;
    }
    if (NULL == creds) {
        creds = api_credentials_new();
        creds->end_point = arguments.service;
        const char *key = api_get_application_key(creds->end_point);
        memcpy((char*)creds->api_key, key, min(MAX_SECRET_LENGTH, strlen(key)));
        const char *secret = api_get_application_secret(creds->end_point);
        memcpy((char*)creds->secret, secret, min(MAX_SECRET_LENGTH, strlen(secret)));
    }
    bool success = true;
    if (arguments.has_url) {
        if (strlen(arguments.url) > 0) {
            strncpy((char*)creds->url, arguments.url, MAX_URL_LENGTH);
        } else {
            _warn("signon::argument_error: missing --url argument");
        }
    }
    if (arguments.disable) {
        _info("signon::disabling: %s", get_api_type_label(arguments.service));
        creds->enabled = false;
    }
    if (arguments.enable) {
        _info("signon::enable: %s", get_api_type_label(arguments.service));
        creds->enabled = true;
    }
    if (arguments.get_token) {
        _info("signon::getting_token: %s", get_api_type_label(arguments.service));

        if (creds->end_point == api_listenbrainz) {
            /*success =*/ set_token(creds);
        } else {
            success = get_token(creds);
        }
    }
    if (arguments.get_session) {
        if (creds->end_point == api_listenbrainz) {
            _warn("signon::getting_session_key: skipping for %s", get_api_type_label(arguments.service));
        } else {
            _info("signon::getting_session_key: %s", get_api_type_label(arguments.service));
            get_session(creds);
        }
    }
    if (!found) {
        arrput(config.credentials, creds);
    }
#if 0
    print_application_config(config);
#endif

    if (success) {
        if (write_credentials_file(&config) == 0) {
            if (arguments.get_session || arguments.disable || arguments.enable) {
                reload_daemon(&config);
            }
            status = EXIT_SUCCESS;
        } else {
            _warn("signon::config_error: unable to write to configuration file");
            status = EXIT_FAILURE;
        }
    } else {
        status = EXIT_FAILURE;
    }

    configuration_clean(&config);

_exit:
    return status;
}
