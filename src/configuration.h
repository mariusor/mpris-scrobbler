/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_CONFIGURATION_H
#define MPRIS_SCROBBLER_CONFIGURATION_H

#define SCROBBLE_BUFF_LEN 2
#ifndef APPLICATION_NAME
#define APPLICATION_NAME            "mpris-scrobbler"
#endif

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
#define TOKENIZED_PID_PATH          "%s/%s%s"
#define TOKENIZED_CREDENTIALS_PATH  "%s/%s/%s"

#define HOME_VAR_NAME               "HOME"
#define USERNAME_VAR_NAME           "USER"
#define XDG_CONFIG_HOME_VAR_NAME    "XDG_CONFIG_HOME"
#define XDG_DATA_HOME_VAR_NAME      "XDG_DATA_HOME"
#define XDG_CACHE_HOME_VAR_NAME     "XDG_CACHE_HOME"
#define XDG_RUNTIME_DIR_VAR_NAME    "XDG_RUNTIME_DIR"

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

void env_variables_free(struct env_variables *env)
{
    if (NULL == env) { return; }

    if (NULL != env->home) { free((char*)env->home); }
    if (NULL != env->user_name) { free((char*)env->user_name); }
    if (NULL != env->xdg_config_home) { free((char*)env->xdg_config_home); }
    if (NULL != env->xdg_data_home) { free((char*)env->xdg_data_home); }
    if (NULL != env->xdg_cache_home) { free((char*)env->xdg_cache_home); }
    if (NULL != env->xdg_runtime_dir) { free((char*)env->xdg_runtime_dir); }

    free(env);
}

struct env_variables *env_variables_new(void)
{
    struct env_variables *env = malloc(sizeof(struct env_variables));
    env->home = NULL;
    env->user_name = NULL;
    env->xdg_data_home = NULL;
    env->xdg_cache_home = NULL;
    env->xdg_config_home = NULL;
    return env;
}

struct configuration *configuration_new(void)
{
    struct configuration *config = malloc(sizeof(struct configuration));
    //config->credentials;
    config->env = env_variables_new();
    config->credentials_length = 0;
    config->name = get_zero_string(128);

    return config;
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

void api_credentials_free(struct api_credentials *credentials)
{
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

static char *get_config_file(struct configuration *config)
{
    if (NULL == config) { return NULL; }
    if (NULL == config->name) { return NULL; }
    if (NULL == config->env) { return NULL; }
    if (NULL == config->env->xdg_config_home) { return NULL; }

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
    if (NULL == config->name) { return NULL; }
    if (NULL == config->env) { return NULL; }
    if (NULL == config->env->xdg_data_home) { return NULL; }

    size_t name_len = strlen(config->name);
    size_t data_home_len = strlen(config->env->xdg_data_home);
    size_t cred_len = strlen(CREDENTIALS_FILE);
    size_t path_len = name_len + data_home_len + cred_len + 2;

    char *path = get_zero_string(path_len);
    if (NULL == path) { return NULL; }

    snprintf(path, path_len + 1, TOKENIZED_CREDENTIALS_PATH, config->env->xdg_data_home, config->name, CREDENTIALS_FILE);

    return path;
}

bool cleanup_pid(const char *path)
{
    if(NULL == path) { return false; }
    return (unlink(path) == 0);
}

char *get_pid_file(struct configuration *config)
{
    if (NULL == config) { return NULL; }
    if (NULL == config->name) { return NULL; }
    if (NULL == config->env) { return NULL; }
    if (NULL == config->env->xdg_runtime_dir) { return NULL; }

    size_t name_len = strlen(config->name);
    size_t ext_len = strlen(PID_SUFFIX);
    size_t runtime_dir_len = strlen(config->env->xdg_runtime_dir);
    size_t path_len = name_len + runtime_dir_len + ext_len + 2;
    char *path = get_zero_string(path_len);
    if (NULL == path) { return NULL; }

    snprintf(path, path_len + 1, TOKENIZED_PID_PATH, config->env->xdg_runtime_dir, config->name, PID_SUFFIX);
    if (NULL == path) { return NULL; }

    return path;
}

typedef struct ini_config ini_config;
void load_credentials_from_ini_group (ini_group *group, struct api_credentials *credentials)
{
    if (credentials) { return; }

    if (strncmp(group->name, SERVICE_LABEL_LASTFM, strlen(SERVICE_LABEL_LASTFM)) == 0) {
        credentials->end_point = lastfm;
    } else if (strncmp(group->name, SERVICE_LABEL_LIBREFM, strlen(SERVICE_LABEL_LIBREFM)) == 0) {
        credentials->end_point = librefm;
    } else if (strncmp(group->name, SERVICE_LABEL_LISTENBRAINZ, strlen(SERVICE_LABEL_LISTENBRAINZ)) == 0) {
        credentials->end_point = listenbrainz;
    }

    for (size_t i = 0; i < group->values_count; i++) {
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
}

bool write_pid(const char *path)
{
    if (NULL == path) { return false; }

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
void load_from_ini_file(struct configuration* config, const char* path)
{
    if (NULL == config) { return; }
    if (NULL == path) { return; }

    FILE *config_file = fopen(path, "r");

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

    ini_config *ini = ini_load(buffer);
    for (size_t i = 0; i < ini->groups_count; i++) {
        ini_group *group = ini->groups[i];
        config->credentials[config->credentials_length] = api_credentials_new();
        load_credentials_from_ini_group(group, config->credentials[config->credentials_length]);
        config->credentials_length++;
    }

    ini_config_free(ini);

_error:
    if (NULL != buffer) { free(buffer); }
    if (NULL != config_file) { fclose(config_file); }
}

void free_configuration(struct configuration *config)
{
    if (NULL == config) { return; }
    _trace("mem::freeing_configuration(%u)", config->credentials_length);
    if (config->credentials_length > 0) {
        for (size_t i = 0 ; i < config->credentials_length; i++) {
            if (NULL != config->credentials[i]) {
                api_credentials_free(config->credentials[i]);
                config->credentials[i] = NULL;
            }
        }
    }
    if (NULL != config->name) { free((char*)config->name); }
    env_variables_free(config->env);
    free(config);
}

bool load_configuration(struct configuration* config, const char *name)
{
    if (NULL == config) { return false; }
    if (NULL == config->env) { return false; }

    if (NULL != name) {
        strncpy((char*)config->name, name, strlen(name));
    }

    load_environment(config->env);

    // reset configuration
    size_t length = config->credentials_length;
    for (size_t j = 0; j < length; j++) {
        if (NULL != config->credentials[j]) {
            api_credentials_free(config->credentials[j]);
            config->credentials_length--;
        }
    }

    char* config_path = get_config_file(config);
    _trace("main::loading_config: %s", config_path);
    load_from_ini_file(config, config_path);
    free(config_path);

    char* credentials_path = get_credentials_cache_file(config);
    _trace("main::loading_credentials: %s", credentials_path);
    load_from_ini_file(config, credentials_path);
    free(credentials_path);

    return true;
}

#endif // MPRIS_SCROBBLER_CONFIGURATION_H
