/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_CONFIGURATION_H
#define MPRIS_SCROBBLER_CONFIGURATION_H

#include <sys/stat.h>

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

static const char *get_api_type_group(enum api_type end_point)
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
        const char *current = environ[i];
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

static char *get_credentials_cache_path(struct configuration *config, const char *file_name)
{
    if (NULL == config) { return NULL; }
    if (NULL == config->name) { return NULL; }
    if (NULL == config->env) { return NULL; }
    if (NULL == config->env->xdg_data_home) { return NULL; }

    bool free_file_name = false;
    if (NULL == file_name) {
        file_name = get_zero_string(0);
        free_file_name = true;
    }

    size_t name_len = strlen(config->name);
    size_t data_home_len = strlen(config->env->xdg_data_home);
    size_t cred_len = strlen(file_name);
    size_t path_len = name_len + data_home_len + cred_len + 2;

    char *path = get_zero_string(path_len);
    if (NULL == path) { return NULL; }

    snprintf(path, path_len + 1, TOKENIZED_CREDENTIALS_PATH, config->env->xdg_data_home, config->name, file_name);

    if (free_file_name) { free((char*)file_name); }

    return path;
}

char *get_credentials_cache_file(struct configuration *config)
{
    return get_credentials_cache_path(config, CREDENTIALS_FILE);
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
bool load_credentials_from_ini_group (ini_group *group, struct api_credentials *credentials)
{
    if (NULL == credentials) { return false; }

    if (strncmp(group->name, SERVICE_LABEL_LASTFM, strlen(SERVICE_LABEL_LASTFM)) == 0) {
        (credentials)->end_point = lastfm;
    } else if (strncmp(group->name, SERVICE_LABEL_LIBREFM, strlen(SERVICE_LABEL_LIBREFM)) == 0) {
        (credentials)->end_point = librefm;
    } else if (strncmp(group->name, SERVICE_LABEL_LISTENBRAINZ, strlen(SERVICE_LABEL_LISTENBRAINZ)) == 0) {
        (credentials)->end_point = listenbrainz;
    }

    for (size_t i = 0; i < group->values_count; i++) {
        ini_value *setting = group->values[i];
        char *key = setting->key;
        char *value = setting->value;
        size_t val_length = strlen(value);
        if (strncmp(key, CONFIG_KEY_ENABLED, strlen(CONFIG_KEY_ENABLED)) == 0) {
            (credentials)->enabled = (strncmp(value, CONFIG_VALUE_FALSE, strlen(CONFIG_VALUE_FALSE)) && strncmp(value, CONFIG_VALUE_ZERO, strlen(CONFIG_VALUE_ZERO)));
        }
        if (strncmp(key, CONFIG_KEY_USER_NAME, strlen(CONFIG_KEY_USER_NAME)) == 0) {
            (credentials)->user_name = get_zero_string(val_length);
            strncpy((credentials)->user_name, value, val_length);
#if 0
            _trace("api::loaded:user_name: %s", (credentials)->user_name);
#endif
        }
        if (strncmp(key, CONFIG_KEY_PASSWORD, strlen(CONFIG_KEY_PASSWORD)) == 0) {
            (credentials)->password = get_zero_string(val_length);
            strncpy((credentials)->password, value, val_length);
#if 0
            _trace("api::loaded:password: %s", (credentials)->password);
#endif
        }
        if (strncmp(key, CONFIG_KEY_TOKEN, strlen(CONFIG_KEY_TOKEN)) == 0) {
            (credentials)->token = get_zero_string(val_length);
            strncpy((credentials)->token, value, val_length);
#if 0
            _trace("api::loaded:token: %s", (credentials)->token);
#endif
        }
        if (strncmp(key, CONFIG_KEY_SESSION, strlen(CONFIG_KEY_SESSION)) == 0) {
            (credentials)->session_key = get_zero_string(val_length);
            strncpy((credentials)->session_key, value, val_length);
#if 0
            _trace("api::loaded:session: %s", (credentials)->session_key);
#endif
        }
    }
    return true;
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
void load_from_ini_file(struct configuration *config, FILE *file)
{
    if (NULL == config) { return; }
    if (NULL == file) { return; }

    char *buffer = NULL;
    if (!file) { goto _error; }

    size_t file_size;
    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    file_size = imax(file_size, MAX_CONF_LENGTH);
    rewind (file);

    buffer = get_zero_string(file_size);
    if (NULL == buffer) { goto _error; }

    if (1 != fread(buffer, file_size, 1, file)) {
        goto _error;
    }

    ini_config *ini = ini_load(buffer);
    for (size_t i = 0; i < ini->groups_count; i++) {
        ini_group *group = ini->groups[i];
#if 0
        _trace("ini::loaded_group: %s, length %lu", group->name, group->values_count);
#endif

        bool found_matching_creds = false;
        struct api_credentials *creds = NULL;
        size_t creds_pos = 0;
        for (size_t j = 0; j < config->credentials_length; j++) {
            creds = config->credentials[j];
            if (strncmp(get_api_type_group(creds->end_point), group->name, strlen(group->name)) == 0) {
                // same group
                found_matching_creds = true;
                creds_pos = j;
                break;
            }
        }
        if (!found_matching_creds) {
            creds = api_credentials_new();
            creds_pos = config->credentials_length;
        }
        if (!load_credentials_from_ini_group(group, creds)) {
            api_credentials_free(creds);
        } else {
            config->credentials[creds_pos] = creds;
        }
        if (!found_matching_creds) {
            config->credentials_length++;
        }
    }
    ini_config_free(ini);

_error:
    if (NULL != buffer) { free(buffer); }
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
#if 0
    memset(&(config->credentials[0]), 0x0, sizeof(struct api_credentials) * config->credentials_length);
    config->credentials_length = 0;
#endif
    if (NULL != config->name) { free((char*)config->name); }
    env_variables_free(config->env);
    free(config);
}

void print_application_config(struct configuration *config)
{
    printf("app::name %s\n", config->name);
    printf("app::user %s\n", config->env->user_name);
    printf("app::home_folder %s\n", config->env->home);
    printf("app::config_folder %s\n", config->env->xdg_config_home);
    printf("app::data_folder %s\n", config->env->xdg_data_home);
    printf("app::cache_folder %s\n", config->env->xdg_cache_home);
    printf("app::runtime_dir %s\n", config->env->xdg_runtime_dir);
    printf("app::loaded_credentials_count %lu\n", config->credentials_length);

    if (config->credentials_length == 0) { return; }
    for (size_t i = 0 ; i < config->credentials_length; i++) {
        struct api_credentials *cur = config->credentials[i];
        printf("app::credentials[%lu]:%s\n", i, get_api_type_label(cur->end_point));
        printf("\tenabled = %s\n", cur->enabled ? "true" : "false" );
        printf("\tauthenticated = %s\n", cur->authenticated ? "true" : "false");
        printf("\tusername = %s\n", cur->user_name);
        printf("\tpassword = %s\n", cur->password);
        printf("\ttoken = %s\n", cur->token);
        printf("\tsession_key = %s\n", cur->session_key);
    }
}

const char *api_get_application_secret(enum api_type);
const char *api_get_application_key(enum api_type);
bool load_configuration(struct configuration *config, const char *name)
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

    char *config_path = get_config_file(config);
    FILE *config_file = fopen(config_path, "r");
    free(config_path);
    if (NULL != config_file) {
        load_from_ini_file(config, config_file);
        fclose(config_file);
        _debug("main::loading_config: ok");
    } else {
        _warn("main::loading_config: failed");
    }

    char *credentials_path = get_credentials_cache_file(config);
    FILE *credentials_file = fopen(credentials_path, "r");
    free(credentials_path);
    if (NULL != credentials_file) {
        load_from_ini_file(config, credentials_file);
        fclose(credentials_file);
        _debug("main::loading_credentials: ok");
    } else {
        _warn("main::loading_credentials: failed");
    }

    for(size_t i = 0; i < config->credentials_length; i++) {
        struct api_credentials *cur = config->credentials[i];
        cur->api_key = api_get_application_key(cur->end_point);
        cur->secret = api_get_application_secret(cur->end_point);
        if (NULL == cur->api_key) {
            _warn("scrobbler::invalid_service[%s]: missing API key", get_api_type_label(cur->end_point));
            cur->enabled = false;
        }
        if (NULL == cur->secret) {
            _warn("scrobbler::invalid_service[%s]: missing API secret key", get_api_type_label(cur->end_point));
            cur->enabled = false;
        }
    }
    return true;
}

bool credentials_folder_exists(const char *path)
{
    if (NULL == path) { return false; }

    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

bool credentials_folder_create(const char *path)
{
    bool status = false;
    const char *err_msg = NULL;

    status = (mkdir(path, S_IRWXU) == 0);
    if (!status) {
        switch(errno) {
            case EACCES:
                err_msg = "Permission denied when writing folder.";
                break;
            case EEXIST:
                err_msg = "The folder already exists.";
                status = true;
                break;
            case ELOOP:
                err_msg = "Unable to resolve folder path.";
                break;
            case EMLINK:
                err_msg = "Link count exceeded.";
                break;
            case ENAMETOOLONG:
                err_msg = "Path length is too long.";
                break;
            case ENOENT:
                err_msg = "Unable to resolve folder path. Missing parent.";
                break;
            case ENOSPC:
                err_msg = "Not enough space to create folder.";
                break;
            case ENOTDIR:
                err_msg = "Parent is not a folder.";
                break;
            case EROFS:
                err_msg = "Parent file-system is read-only.";
                break;
            default:
                err_msg = "Unknown error";
        }
        _trace("credentials::folder_error: %s", err_msg);
    }
    return status;
}

int write_credentials_file(struct configuration *config)
{
    int status = -1;
    char *file_path = NULL;
    char *folder_path = NULL;
    struct ini_config *to_write = NULL;

    folder_path = get_credentials_cache_path(config, NULL);
    if (!credentials_folder_exists(folder_path) && !credentials_folder_create(folder_path)) {
        _error("main::credentials: Unable to create data folder %s", folder_path);
        goto _return;
    }
#if 0
    if (!credentials_folder_valid(folder_path)) {
        _warn("main::credentials: wrong permissions for folder %s, should be 'rwx------'")
    }
#endif
    to_write = get_ini_from_credentials(config->credentials, config->credentials_length);
    file_path = get_credentials_cache_file(config);

    if (NULL != file_path) {
        _debug("saving::credentials[%u]: %s", config->credentials_length, file_path);
        FILE *file = fopen(file_path, "w+");
        status = write_ini_file(to_write, file);
    }

_return:
    if (NULL != to_write) { ini_config_free(to_write); }
    if (NULL != file_path) { free(file_path); }
    if (NULL != folder_path) { free(folder_path); }

    return status;
}

#endif // MPRIS_SCROBBLER_CONFIGURATION_H
