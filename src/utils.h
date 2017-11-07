/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_UTILS_H
#define MPRIS_SCROBBLER_UTILS_H

#include <event.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define array_count(a) (sizeof(a)/sizeof 0[a])

#define SCROBBLE_BUFF_LEN 2
#ifndef APPLICATION_NAME
#define APPLICATION_NAME "mpris-scrobbler"
#endif

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

enum log_levels
{
    none    = 0,
    error   = (1 << 0),
    warning = (1 << 1),
    info    = (1 << 2),
    debug   = (1 << 3),
    tracing = (1 << 4),
};

static const char* get_log_level (enum log_levels l) {
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

static bool level_is(unsigned incoming, enum log_levels level)
{
    return ((incoming & level) == level);
}

#define _error(...) _log(error, __VA_ARGS__)
#define _warn(...) _log(warning, __VA_ARGS__)
#define _info(...) _log(info, __VA_ARGS__)
#define _debug(...) _log(debug, __VA_ARGS__)
#define _trace(...) _log(tracing, __VA_ARGS__)

static int _log(enum log_levels level, const char* format, ...)
{
#ifndef DEBUG
    if (level_is(level, tracing)) { return 0; }
#endif

    extern enum log_levels _log_level;
    //printf("__TRACE: log level r[%u] %s -> g[%u] %s is %s [%u] \n", level, get_log_level(level), _log_level, get_log_level(_log_level), (level < _log_level) ? "smaller" : "greater", (level & _log_level));
    //return 0;
    if (!level_is(_log_level, level)) { return 0; }

    va_list args;
    va_start(args, format);

    const char *label = get_log_level(level);
    size_t p_len = strlen(LOG_WARNING_LABEL) + 1;

    const char* suffix = "\n";
    size_t s_len = strlen(suffix);
    size_t f_len = strlen(format);
    size_t full_len = p_len + f_len + s_len + 1;

    char log_format[full_len];
    memset(log_format, 0x0, full_len);
    snprintf(log_format, p_len + 1, "%-7s ", label);

    strncpy(log_format + p_len, format, f_len);
    strncpy(log_format + p_len + f_len, suffix, s_len);

    int result = vfprintf(stderr, log_format, args);
    va_end(args);

    return result;
}

size_t string_trim(char **string, size_t len, const char *remove)
{
    if (NULL == *string) { return 0; }
    const char *default_chars = "\t\r\n ";

    if (NULL == remove) {
        remove = (char*)default_chars;
    }

    size_t st_pos = 0;
    size_t end_pos = len - 1;
    for(size_t j = 0; j < strlen(remove); j++) {
        char m_char = remove[j];
        for (size_t i = 0; i < len; i++) {
            char byte = (*string)[i];
            if (byte == m_char) {
                st_pos = i;
                continue;
            }
        }
        if (st_pos < end_pos) {
            for (int i = end_pos; i >= 0; i--) {
                char byte = (*string)[i];
                if (byte == m_char) {
                    end_pos = i;
                    continue;
                }
            }
        }
    }
    int new_len = end_pos - st_pos;
    if (st_pos > 0 && end_pos > 0 && new_len > 0) {
        char *new_string = get_zero_string(new_len);
        strncpy(new_string, *string + st_pos, new_len);
        string = &new_string;
    }

    return new_len;
}

static const char* get_api_type_label(enum api_type end_point)
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

static const char* get_api_type_group(enum api_type end_point)
{
    const char *api_label;
    switch (end_point) {
        case(lastfm):
            api_label = "lastfm";
        break;
        case(librefm):
            api_label = "librefm";
        break;
        case(listenbrainz):
            api_label = "listenbrainz";
        break;
        default:
            api_label = NULL;
        break;
    }

    return api_label;
}

struct api_credentials *api_credentials_new(void)
{
    struct api_credentials *credentials = malloc(sizeof(struct api_credentials));
    if (NULL == credentials) { return NULL; }

    credentials->end_point = unknown;
    credentials->enabled = false;
    credentials->authenticated = false;
    credentials->token = NULL;
    credentials->session_key = NULL;
    credentials->user_name = NULL;
    credentials->password = NULL;

    return credentials;
}

void api_credentials_free(struct api_credentials *credentials) {
    if (NULL == credentials) { return; }
    if (credentials->enabled) {
        _trace("mem::freeing_credentials(%p): %s", credentials, get_api_type_label(credentials->end_point));
    }
    if (NULL != credentials->user_name) { free(credentials->user_name); }
    if (NULL != credentials->password)  { free(credentials->password); }
    if (NULL != credentials->token)  { free(credentials->token); }
    if (NULL != credentials->session_key)  { free(credentials->session_key); }
    free(credentials);
}

void free_configuration(struct configuration *global_config)
{
    if (NULL == global_config) { return; }
    _trace("mem::freeing_configuration(%u)", global_config->credentials_length);
    for (size_t i = 0 ; i < global_config->credentials_length; i++) {
        struct api_credentials *credentials = global_config->credentials[i];
        if (NULL != credentials) { api_credentials_free(credentials); }
    }
    if (NULL != global_config->name) { free(global_config->name); }
    //free(global_config);
}

void zero_string(char** incoming, size_t length)
{
    if (NULL == incoming) { return; }
    size_t length_with_null = (length + 1) * sizeof(char);
    memset(*incoming, 0, length_with_null);
}

#define PID_SUFFIX                  ".pid"
#define CONFIG_FILE_SUFFIX          ".ini"
#define CREDENTIALS_FILE            "credentials"
#define CONFIG_DIR_NAME             ".config"
#define CACHE_DIR_NAME              ".cache"
#define DATA_DIR_NAME               ".local/share"

#define TOKENIZED_CONFIG_DIR        "%s/%s"
#define TOKENIZED_DATA_DIR          "%s/%s"
#define TOKENIZED_CACHE_DIR         "%s/%s"
#define TOKENIZED_CONFIG_PATH       "%s/%s%s"
#define TOKENIZED_CREDENTIALS_PATH  "%s/%s/%s"

#define HOME_VAR_NAME               "HOME"
#define USERNAME_VAR_NAME           "USER"
#define XDG_CONFIG_HOME_VAR_NAME    "XDG_CONFIG_HOME"
#define XDG_DATA_HOME_VAR_NAME      "XDG_DATA_HOME"
#define XDG_CACHE_HOME_VAR_NAME     "XDG_CACHE_HOME"
#define XDG_RUNTIME_DIR_VAR_NAME    "XDG_RUNTIME_DIR"


static void load_environment(struct env_variables *env)
{
    if (NULL == env) { return; }
    extern char **environ;

    size_t home_var_len = strlen(HOME_VAR_NAME);
    size_t username_var_len = strlen(USERNAME_VAR_NAME);
    size_t config_home_var_len = strlen(XDG_CONFIG_HOME_VAR_NAME);
    size_t data_home_var_len = strlen(XDG_DATA_HOME_VAR_NAME);
    size_t cache_home_var_len = strlen(XDG_CACHE_HOME_VAR_NAME);
    size_t runtime_dir_var_len = strlen(XDG_RUNTIME_DIR_VAR_NAME);

    size_t home_len = 0;
    size_t username_len = 0;
    size_t config_home_len = 0;
    size_t data_home_len = 0;
    size_t cache_home_len = 0;
    size_t runtime_dir_len = 0;

    size_t i = 0;
    while(environ[i]) {
        const char* current = environ[i];
        size_t current_len = strlen(current);
        if (strncmp(current, HOME_VAR_NAME, home_var_len) == 0) {
            home_len = current_len - home_var_len;
            env->home = get_zero_string(home_len);
            if (NULL == env->home) { continue; }
            strncpy((char*)env->home, current + home_var_len + 1, home_len);
        }
        if (strncmp(current, USERNAME_VAR_NAME, username_var_len) == 0) {
            username_len = current_len - username_var_len;
            env->user_name = get_zero_string(username_len);
            if (NULL == env->user_name) { continue; }
            strncpy((char*)env->user_name, current + username_var_len + 1, username_len);
        }
        if (strncmp(current, XDG_CONFIG_HOME_VAR_NAME, config_home_var_len) == 0) {
            config_home_len = current_len - config_home_var_len;
            env->xdg_config_home  = get_zero_string(config_home_len);
            if (NULL == env->xdg_config_home) { continue; }
            strncpy((char*)env->xdg_config_home, current + config_home_var_len + 1, config_home_len);
        }
        if (strncmp(current, XDG_DATA_HOME_VAR_NAME, data_home_var_len) == 0) {
            data_home_len = current_len - data_home_var_len;
            env->xdg_data_home  = get_zero_string(data_home_len);
            if (NULL == env->xdg_data_home) { continue; }
            strncpy((char*)env->xdg_data_home, current + data_home_var_len + 1, data_home_len);
        }
        if (strncmp(current, XDG_CACHE_HOME_VAR_NAME, cache_home_var_len) == 0) {
            cache_home_len = current_len - cache_home_var_len;
            env->xdg_cache_home  = get_zero_string(cache_home_len);
            if (NULL == env->xdg_cache_home) { continue; }
            strncpy((char*)env->xdg_cache_home, current + cache_home_var_len + 1, cache_home_len);
        }
        if (strncmp(current, XDG_RUNTIME_DIR_VAR_NAME, runtime_dir_var_len) == 0) {
            runtime_dir_len = current_len - runtime_dir_var_len;
            env->xdg_runtime_dir  = get_zero_string(runtime_dir_len);
            if (NULL == env->xdg_runtime_dir) { continue; }
            strncpy((char*)env->xdg_runtime_dir, current + runtime_dir_var_len + 1, runtime_dir_len);
        }
        i++;
    }
    if (NULL != env->home) {
        if (NULL == env->xdg_data_home) {
            data_home_len = strlen(env->home) + strlen(CONFIG_DIR_NAME) + 1;
            env->xdg_data_home = get_zero_string(data_home_len);
            snprintf((char*)env->xdg_data_home, data_home_len, TOKENIZED_DATA_DIR, env->home, CONFIG_DIR_NAME);
        }
        if (NULL == env->xdg_config_home) {
            config_home_len = strlen(env->home) + strlen(DATA_DIR_NAME) + 1;
            env->xdg_config_home = get_zero_string(config_home_len + 1);
            snprintf((char*)env->xdg_config_home, config_home_len, TOKENIZED_CONFIG_DIR, env->home, DATA_DIR_NAME);
        }
        if (NULL == env->xdg_cache_home) {
            cache_home_len = strlen(env->home) + strlen(CACHE_DIR_NAME) + 1;
            env->xdg_cache_home = get_zero_string(cache_home_len + 1);
            snprintf((char*)env->xdg_cache_home, cache_home_len, TOKENIZED_CACHE_DIR, env->home, CACHE_DIR_NAME);
        }
    }
}

void env_variables_free(struct env_variables *env)
{
    if (NULL == env) { return; }
    if (NULL != env->home) { free((char*)env->home); }
    if (NULL != env->user_name) { free((char*)env->user_name); }
    if (NULL != env->xdg_config_home) { free((char*)env->xdg_config_home); }
    if (NULL != env->xdg_data_home) { free((char*)env->xdg_data_home); }
    if (NULL != env->xdg_config_home) { free((char*)env->xdg_config_home); }
    if (NULL != env->xdg_runtime_dir) { free((char*)env->xdg_runtime_dir); }

    free(env);
}

struct env_variables *env_variables_new(void)
{
    struct env_variables *env = calloc(1, sizeof(struct env_variables));
    return env;
}

static char *get_config_file(struct configuration *config)
{
    if (NULL == config) { return NULL; }

    size_t name_len = strlen(config->name);
    size_t config_home_len = strlen(config->env->xdg_config_home);
    size_t path_len = name_len + config_home_len + strlen(CONFIG_FILE_SUFFIX) + 1;
    char *path = get_zero_string(path_len);
    if (NULL == path) { return NULL; }

    snprintf(path, path_len + 1, TOKENIZED_CONFIG_PATH, config->env->xdg_config_home, config->name, CONFIG_FILE_SUFFIX);
    if (NULL == path) { return NULL; }

    return path;
}

char *get_credentials_cache_file(struct configuration *config)
{
    if (NULL == config) { return NULL; }

    size_t name_len = strlen(config->name);
    size_t data_home_len = strlen(config->env->xdg_data_home);
    size_t path_len = name_len + data_home_len + strlen(CREDENTIALS_FILE) + 2;
    char *path = get_zero_string(path_len);
    if (NULL == path) { return NULL; }

    snprintf(path, path_len + 1, TOKENIZED_CREDENTIALS_PATH, config->env->xdg_data_home, config->name, CREDENTIALS_FILE);
    if (NULL == path) { return NULL; }

    return path;
}

char *get_full_pid_path(const char* name, char* dir_path)
{
    size_t len = 0;
    bool free_dir = false;
    //_error("fucking path %s %s", name, dir_path);
    if (NULL == dir_path) {
        // load the pid path from environment variables
        extern char **environ;
        size_t xdg_runtime_dir_var_len = strlen(XDG_RUNTIME_DIR_VAR_NAME);
        size_t i = 0;
        while(environ[i]) {
            const char* current = environ[i];
            size_t current_len = strlen(current);
            if (strncmp(current, XDG_RUNTIME_DIR_VAR_NAME, xdg_runtime_dir_var_len) == 0) {
                len = current_len - xdg_runtime_dir_var_len;

                dir_path = get_zero_string(len);
                strncpy(dir_path, current + xdg_runtime_dir_var_len + 1, len);
                strncat(dir_path, "/", 1);

                if (NULL == dir_path) { break; }
                free_dir = true;
            }
            i++;
        }
    } else {
        len = strlen(dir_path);
    }

    char *path;
    size_t name_len = strlen(name);
    size_t ext_len = strlen(PID_SUFFIX);

    path = get_zero_string(len + name_len + ext_len);
    if (NULL == path) { goto _error; }

    strncpy(path, dir_path, len);

    strncat(path, name, name_len);
    strncat(path, PID_SUFFIX, ext_len);

    char* r_path = get_zero_string(MAX_PROPERTY_LENGTH);
    r_path = realpath(path, r_path);

    free (dir_path);
    return r_path;
_error:
    if (free_dir) { free(dir_path); }
    return NULL;
}

#define CONFIG_VALUE_TRUE           "true"
#define CONFIG_VALUE_ONE            "1"
#define CONFIG_VALUE_FALSE          "false"
#define CONFIG_VALUE_ZERO           "0"
#define CONFIG_KEY_ENABLED          "enabled"
#define CONFIG_KEY_USER_NAME        "username"
#define CONFIG_KEY_PASSWORD         "password"
#define CONFIG_KEY_TOKEN            "token"
#define CONFIG_KEY_SESSION          "session"
#define SERVICE_LABEL_LASTFM        "lastfm"
#define SERVICE_LABEL_LIBREFM       "librefm"
#define SERVICE_LABEL_LISTENBRAINZ  "listenbrainz"

typedef struct ini_config ini_config;
static struct api_credentials *load_credentials_from_ini_group (ini_group *group)
{
    struct api_credentials *credentials = api_credentials_new();

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
            credentials->enabled = (strncmp(value, CONFIG_VALUE_FALSE, strlen(CONFIG_VALUE_FALSE)) && strncmp(value, CONFIG_VALUE_ZERO, strlen(CONFIG_VALUE_ZERO)));
        }
        if (strncmp(key, CONFIG_KEY_USER_NAME, strlen(CONFIG_KEY_USER_NAME)) == 0) {
            credentials->user_name = get_zero_string(val_length);
            strncpy(credentials->user_name, value, val_length);
        }
        if (strncmp(key, CONFIG_KEY_PASSWORD, strlen(CONFIG_KEY_PASSWORD)) == 0) {
            credentials->password = get_zero_string(val_length);
            strncpy(credentials->password, value, val_length);
        }
        if (strncmp(key, CONFIG_KEY_TOKEN, strlen(CONFIG_KEY_TOKEN)) == 0) {
            credentials->token = get_zero_string(val_length);
            strncpy(credentials->token, value, val_length);
        }
        if (strncmp(key, CONFIG_KEY_SESSION, strlen(CONFIG_KEY_SESSION)) == 0) {
            credentials->session_key = get_zero_string(val_length);
            strncpy(credentials->session_key, value, val_length);
        }
    }
    if (!credentials->enabled) {
        api_credentials_free(credentials);
        return NULL;
    }
    if (NULL != credentials->password) {
        size_t pl_len = strlen(credentials->password);
        char pass_label[256];

        for (size_t i = 0; i < pl_len; i++) {
            pass_label[i] = '*';
        }
        pass_label[pl_len] = '\0';
        _debug("main::load_credentials(%s): %s:%s", get_api_type_label(credentials->end_point), credentials->user_name, pass_label);
    } else {
        _debug("main::load_credentials(%s)", get_api_type_label(credentials->end_point));
    }


    return credentials;
}

size_t load_application_name(struct configuration *config, const char *first_arg, size_t first_arg_len) {
    if (NULL == first_arg) { return 0; }
    if (first_arg_len == 0) { return 0; }

    config->name = get_zero_string(first_arg_len);
    strncpy(config->name, first_arg, first_arg_len);

    //size_t new_length = string_trim(&(config->name), first_arg_len, "./");

    _trace("main::app_name %s", config->name);

    return first_arg_len;
}

bool write_pid(const char *path)
{
    if (NULL == path) { return false; }

    char* r_path = get_zero_string(MAX_PROPERTY_LENGTH);
    r_path = realpath(path, r_path);

    FILE *pidfile = fopen(path, "w");
    if (NULL == pidfile) {
        _warn("main::invalid_pid_path %s", path);
        return false;
    }
    fprintf(pidfile, "%d", getpid());
    fclose(pidfile);
    return true;
}

#define MAX_CONF_LENGTH 1024
#define imax(a, b) ((a > b) ? b : a)

bool load_configuration(struct configuration* config, const char* name)
{
    if (NULL == config) { return false; }
    if (NULL == config->name) {
        load_application_name(config, name, strlen(name));
    }
    config->env = env_variables_new();
    load_environment(config->env);

    char* path = get_config_file(config);
    FILE *config_file = fopen(path, "r");
    free(path);

    char *buffer = NULL;
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
    size_t length = config->credentials_length;

    for (size_t j = 0; j < length; j++) {
        if (NULL != config->credentials[j]) {
            api_credentials_free(config->credentials[j]);
            config->credentials_length--;
        }
    }

    ini_config *ini = ini_load(buffer);
    for (size_t i = 0; i < ini->length; i++) {
        ini_group *group = ini->groups[i];
        struct api_credentials *temp = load_credentials_from_ini_group(group);

        if (NULL != temp) {
            config->credentials[config->credentials_length] = temp;
            config->credentials_length++;
        }
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
        load_configuration(&global_config, global_config.name);
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

bool cleanup_pid(const char *path)
{
    if(NULL == path) { return false; }
    return (unlink(path) == 0);
}

struct ini_config *get_ini_from_credentials(struct api_credentials *credentials[], size_t length)
{
    if (NULL == credentials) { return NULL; }
    if (length == 0) { return NULL; }

    struct ini_config *creds_config = ini_config_new();
    if (NULL == creds_config) { return NULL; }

    for (size_t i = 0; i < length; i++) {
        struct api_credentials *current = credentials[i];
        if (NULL == current) { continue; }
        if (current->end_point == unknown) { continue; }

        char *label = (char*)get_api_type_group(current->end_point);
        struct ini_group *group = ini_group_new(label);

        if (NULL != current->token) {
            struct ini_value *token = ini_value_new(CONFIG_KEY_TOKEN, current->token);
            ini_group_append_value(group, token);
        }
        if (NULL != current->session_key) {
            struct ini_value *session_key = ini_value_new(CONFIG_KEY_SESSION, current->session_key);
            ini_group_append_value(group, session_key);
        }

        ini_config_append_group(creds_config, group);
    }

    return creds_config;
}

#endif // MPRIS_SCROBBLER_UTILS_H
