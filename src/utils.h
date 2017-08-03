/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <event.h>

#define array_count(a) (sizeof(a)/sizeof 0[a])

#define SCROBBLE_BUFF_LEN 2

#define APPLICATION_NAME "mpris-scrobbler"
#define CREDENTIALS_PATH APPLICATION_NAME "/credentials"

char* get_zero_string(size_t length)
{
    size_t length_with_null = (length + 1) * sizeof(char);
    char* result = (char*)calloc(1, length_with_null);
#if 0
    if (NULL == result) {
        _trace("mem::could_not_allocate %lu bytes string", length_with_null);
    } else {
        _trace("mem::allocated %lu bytes string", length_with_null);
    }
#endif

    return result;
}

#define LOG_ERROR_LABEL "ERROR"
#define LOG_WARNING_LABEL "WARNING"
#define LOG_DEBUG_LABEL "DEBUG"
#define LOG_INFO_LABEL "INFO"
#define LOG_TRACING_LABEL "TRACING"

typedef enum log_levels
{
    none = (1 << 0),
    tracing = (1 << 1),
    debug   = (1 << 2),
    info    = (1 << 3),
    warning = (1 << 4),
    error   = (1 << 5)
} log_level;

static const char* get_log_level (log_level l) {
    switch (l) {
        case (error):
            return LOG_ERROR_LABEL;
        case (warning):
            return LOG_WARNING_LABEL;
        case (debug):
            return LOG_DEBUG_LABEL;
        case (tracing):
            return LOG_TRACING_LABEL;
        case (info):
        default:
            return LOG_INFO_LABEL;
    }
}

#define _error(...) _log(error, __VA_ARGS__)
#define _warn(...) _log(warning, __VA_ARGS__)
#define _info(...) _log(info, __VA_ARGS__)
#define _debug(...) _log(debug, __VA_ARGS__)
#define _trace(...) _log(tracing, __VA_ARGS__)

static int _log(log_level level, const char* format, ...)
{
#ifndef DEBUG
    if (level == debug || level == tracing) { return 0; }
#endif

    extern log_level _log_level;
    if (level < _log_level) { return 0; }

    va_list args;
    va_start(args, format);

    const char *label = get_log_level(level);
    size_t l_len = strlen(LOG_WARNING_LABEL);

    size_t p_len = l_len + 1;
    char* preffix = get_zero_string(p_len);
    snprintf(preffix, p_len + 1, "%-7s ", label);

    char* suffix = "\n";
    size_t s_len = strlen(suffix);
    size_t f_len = strlen(format);
    size_t full_len = p_len + f_len + s_len;

    char* log_format = get_zero_string(full_len);
    strncpy(log_format, preffix, p_len);
    free(preffix);

    strncpy(log_format + p_len, format, f_len);
    strncpy(log_format + p_len + f_len, suffix, s_len);

    int result = vfprintf(stderr, log_format, args);
    free(log_format);
    va_end(args);

    return result;
}

static void free_credentials(struct api_credentials *credentials) {
    if (NULL == credentials) { return; }
    if (NULL != credentials->user_name) { free(credentials->user_name); }
    if (NULL != credentials->password)  { free(credentials->password); }
    free(credentials);
}

void free_configuration(struct configuration *global_config)
{
    if (NULL == global_config) { return; }
    for (size_t i = 0 ; i < global_config->credentials_length; i++) {
        free_credentials(global_config->credentials[i]);
    }
    //free(global_config);
}

void zero_string(char** incoming, size_t length)
{
    if (NULL == incoming) { return; }
    size_t length_with_null = (length + 1) * sizeof(char);
    memset(*incoming, 0, length_with_null);
}

#define CONFIG_DIR_NAME ".config"
#define TOKENIZED_HOME_USER_PATH "%s/%s/%s"
#define TOKENIZED_XDG_CONFIG_PATH "%s/%s"
#define HOME_VAR_NAME "HOME"
#define USERNAME_VAR_NAME "USER"
#define XDG_CONFIG_HOME_VAR_NAME "XDG_CONFIG_HOME"
#define XDG_DATA_HOME_VAR_NAME "XDG_DATA_HOME"

static FILE *get_config_file(const char* path, const char* mode)
{
    extern char **environ;
    char* config_path = NULL;
    char* home_path = NULL;
    char* username = NULL;
    char* config_home_path = NULL;
    char* data_home_path = NULL;

    size_t home_var_len = strlen(HOME_VAR_NAME);
    size_t username_var_len = strlen(USERNAME_VAR_NAME);
    size_t config_home_var_len = strlen(XDG_CONFIG_HOME_VAR_NAME);
    size_t data_home_var_len = strlen(XDG_DATA_HOME_VAR_NAME);

    size_t home_len = 0;
    size_t username_len = 0;
    size_t config_home_len = 0;
    size_t data_home_len = 0;

    size_t path_len = strlen(path);

    int i = 0;
    while(environ[i]) {
        const char* current = environ[i];
        size_t current_len = strlen(current);
        if (strncmp(current, HOME_VAR_NAME, home_var_len) == 0) {
            home_len = current_len - home_var_len;
            home_path = get_zero_string(home_len);
            if (NULL == home_path) { continue; }
            strncpy(home_path, current + home_var_len + 1, home_len);
        }
        if (strncmp(current, USERNAME_VAR_NAME, username_var_len) == 0) {
            username_len = current_len - username_var_len;
            username = get_zero_string(username_len);
            if (NULL == username) { continue; }
            strncpy(username, current + username_var_len + 1, username_len);
        }
        if (strncmp(current, XDG_CONFIG_HOME_VAR_NAME, config_home_var_len) == 0) {
            config_home_len = current_len - config_home_var_len;
            config_home_path  = get_zero_string(config_home_len);
            if (NULL == config_home_path) { continue; }
            strncpy(config_home_path, current + config_home_var_len + 1, config_home_len);
        }
        if (strncmp(current, XDG_DATA_HOME_VAR_NAME, data_home_var_len) == 0) {
            data_home_len = current_len - data_home_var_len;
            data_home_path  = get_zero_string(data_home_len);
            if (NULL == data_home_path) { continue; }
            strncpy(data_home_path, current + data_home_var_len + 1, data_home_len);
        }
        i++;
    }

    if (NULL != config_home_path) {
        size_t full_path_len = config_home_len + path_len + 1;
        config_path = get_zero_string(full_path_len + 1);
        snprintf(config_path, full_path_len, TOKENIZED_XDG_CONFIG_PATH, config_home_path, path);
    } else {
        if (NULL != username && NULL != home_path) {
            size_t full_path_len = home_len + username_len + path_len + 2;
            config_path = get_zero_string(full_path_len + 1);
            snprintf(config_path, full_path_len, TOKENIZED_HOME_USER_PATH, home_path, username, path);
        }
    }
    if (NULL != home_path) { free(home_path); }
    if (NULL != username) { free(username); }
    if (NULL != config_home_path) { free(config_home_path); }
    if (NULL != data_home_path) { free(data_home_path); }
    if (NULL == config_path) { return NULL; }

    FILE *result = fopen(config_path, mode);

    if (NULL != config_path) { free(config_path); }

    return result;
}

static const char* get_api_type_label(api_type end_point)
{
    const char *api_label;
    switch (end_point) {
        case(lastfm):
            api_label = "last.fm";
        break;
        case(librefm):
            api_label = "libre.fm";
        break;
        case(listenbrainz):
            api_label = "listenbrainz.org";
        break;
        default:
            api_label = "error";
        break;
    }

    return api_label;
}

#define CONFIG_KEY_ENABLED "enabled"
#define CONFIG_ENABLED_VALUE_TRUE "true"
#define CONFIG_ENABLED_VALUE_ONE "1"
#define CONFIG_ENABLED_VALUE_FALSE "false"
#define CONFIG_ENABLED_VALUE_ZERO "0"
#define CONFIG_KEY_USER_NAME "username"
#define CONFIG_KEY_PASSWORD "password"
#define SERVICE_LABEL_LASTFM "lastfm"
#define SERVICE_LABEL_LIBREFM "librefm"
#define SERVICE_LABEL_LISTENBRAINZ "listenbrainz"

typedef struct ini_config ini_config;
static struct api_credentials *load_credentials_from_ini_group (ini_group *group)
{
    struct api_credentials *credentials = calloc(1, sizeof(struct api_credentials));
    credentials->enabled = true;
    if (strncmp(group->name, SERVICE_LABEL_LASTFM, strlen(SERVICE_LABEL_LASTFM)) == 0) {
        credentials->end_point = lastfm;
    } else if (strncmp(group->name, SERVICE_LABEL_LIBREFM, strlen(SERVICE_LABEL_LIBREFM)) == 0) {
        credentials->end_point = librefm;
    } else if (strncmp(group->name, SERVICE_LABEL_LISTENBRAINZ, strlen(SERVICE_LABEL_LISTENBRAINZ)) == 0) {
        credentials->end_point = listenbrainz;
    }

    for (size_t i = 0; i < group->length; i++) {
        ini_value *setting = group->values[i];
        char *key = setting->key;
        char *value = setting->value;
        size_t val_length = strlen(value);
        if (strncmp(key, CONFIG_KEY_ENABLED, strlen(CONFIG_KEY_ENABLED)) == 0) {
            credentials->enabled = (strncmp(value, CONFIG_ENABLED_VALUE_FALSE, strlen(CONFIG_ENABLED_VALUE_FALSE)) && strncmp(value, CONFIG_ENABLED_VALUE_ZERO, strlen(CONFIG_ENABLED_VALUE_ZERO)));
        }
        if (strncmp(key, CONFIG_KEY_USER_NAME, strlen(CONFIG_KEY_USER_NAME)) == 0) {
            credentials->user_name = get_zero_string(val_length);
            strncpy(credentials->user_name, value, val_length);
        }
        if (strncmp(key, CONFIG_KEY_PASSWORD, strlen(CONFIG_KEY_PASSWORD)) == 0) {
            credentials->password = get_zero_string(val_length);
            strncpy(credentials->password, value, val_length);
        }
    }
    if (!credentials->enabled) { return NULL; }
    size_t pl_len = strlen(credentials->password);
    char pass_label[256];

    for (size_t i = 0; i < pl_len; i++) {
        pass_label[i] = '*';
    }
    pass_label[pl_len] = '\0';

    _debug("main::load_credentials(%s): %s:%s", get_api_type_label(credentials->end_point), credentials->user_name, pass_label);

    return credentials;
}

#define MAX_CONF_LENGTH 1024
#define imax(a, b) ((a > b) ? b : a)

bool load_configuration(struct configuration* global_config)
{
    if (NULL == global_config) { return false; }
    const char *path = CREDENTIALS_PATH;
    char *buffer = NULL;

    FILE *config_file = get_config_file(path, "r");

    if (!config_file) { goto _error; }

    size_t file_size;

    fseek(config_file, 0L, SEEK_END);
    file_size = ftell(config_file);
    file_size = imax(file_size, MAX_CONF_LENGTH);
    rewind (config_file);

    buffer = get_zero_string(file_size);
    if (NULL == buffer) { goto _error; }

    if (1 != fread(buffer, file_size, 1, config_file)) {
        goto _error;
    }
    fclose(config_file);

    ini_config *ini = ini_load(buffer);
    for (size_t i = 0; i < ini->length; i++) {
        ini_group *group = ini->groups[i];
        free_credentials(global_config->credentials[i]);
        global_config->credentials[i] = load_credentials_from_ini_group(group);
        global_config->credentials_length++;
    }

    ini_config_free(ini);
    free(buffer);
    return true;
_error:
    if (NULL != buffer) { free(buffer); }
    if (NULL != config_file) { fclose(config_file); }
    _error("main::load_config: failed");
    return false;
}

void sighandler(evutil_socket_t signum, short events, void *user_data)
{
    if (events) { events = 0; }
    struct event_base *eb = user_data;

    const char* signal_name = "UNKNOWN";
    switch (signum) {
        case SIGHUP:
            signal_name = "SIGHUP";
            break;
        case SIGINT:
            signal_name = "SIGINT";
            break;
        case SIGTERM:
            signal_name = "SIGTERM";
            break;

    }
    _info("main::signal_received: %s", signal_name);

    if (signum == SIGHUP) {
        extern struct configuration global_config;
        load_configuration(&global_config);
    }
    if (signum == SIGINT || signum == SIGTERM) {
        event_base_loopexit(eb, NULL);
    }
}

void cpstr(char** d, const char* s, size_t max_len)
{
    if (NULL == s) { return; }

    size_t t_len = strlen(s);
    if (t_len > max_len) { t_len = max_len; }
    if (t_len == 0) { return; }

    *d = get_zero_string(t_len);
    if (NULL == *d) { return; }
    strncpy(*d, s, t_len);

#if 0
    _trace("mem::cpstr(%p->%p:%u): %s", s, *d, t_len, s);
#endif
}
#endif // UTILS_H
