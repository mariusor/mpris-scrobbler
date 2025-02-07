/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_CONFIGURATION_H
#define MPRIS_SCROBBLER_CONFIGURATION_H

#include <string.h>
#include <sys/stat.h>
#include "ini.h"

#ifndef APPLICATION_NAME
#define APPLICATION_NAME            "mpris-scrobbler"
#endif

#define PID_SUFFIX                  ".pid"
#define CREDENTIALS_FILE_NAME       "credentials"
#define CACHE_FILE_NAME             "queue"
#define CONFIG_FILE_NAME            "config"
#define CONFIG_DIR_NAME             ".config"
#define CACHE_DIR_NAME              ".cache"
#define DATA_DIR_NAME               ".local/share"
#define HOME_DIR                    "/home"

#define TOKENIZED_CONFIG_DIR        "%s/%s"
#define TOKENIZED_DATA_DIR          "%s/%s"
#define TOKENIZED_CACHE_DIR         "%s/%s"
#define TOKENIZED_CONFIG_PATH       "%s/%s/%s"
#define TOKENIZED_PID_PATH          "%s/%s%s"
#define TOKENIZED_CREDENTIALS_PATH  "%s/%s/%s"
#define TOKENIZED_CACHE_PATH        "%s/%s/%s"

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
#define CONFIG_KEY_URL              "url"
#define SERVICE_LABEL_LASTFM        "lastfm"
#define SERVICE_LABEL_LIBREFM       "librefm"
#define SERVICE_LABEL_LISTENBRAINZ  "listenbrainz"
#define CONFIG_KEY_IGNORE           "ignore"

static const char *get_api_type_group(enum api_type end_point)
{
    const char *api_label = NULL;
    switch (end_point) {
        case(api_lastfm):
            api_label = "lastfm";
            break;
        case(api_librefm):
            api_label = "librefm";
            break;
        case(api_listenbrainz):
            api_label = "listenbrainz";
            break;
        case(api_unknown):
        default:
            api_label = NULL;
            break;
    }

    return api_label;
}

static struct ini_config *get_ini_from_credentials(struct api_credentials *credentials[], const size_t length)
{
    if (NULL == credentials) { return NULL; }
    if (length == 0) { return NULL; }

    struct ini_config *creds_config = ini_config_new();
    if (NULL == creds_config) { return NULL; }

    for (size_t i = 0; i < length; i++) {
        struct api_credentials *current = credentials[i];
        if (NULL == current) { continue; }
        if (current->end_point == api_unknown) { continue; }

        const char *label = get_api_type_group(current->end_point);
        struct ini_group *group = ini_group_new(label);

        struct ini_value *enabled = ini_value_new(CONFIG_KEY_ENABLED, current->enabled ? "true" : "false");
        ini_group_append_value(group, enabled);

        if (strlen(current->user_name) > 0) {
            struct ini_value *user_name = ini_value_new(CONFIG_KEY_USER_NAME, (char*)current->user_name);
            ini_group_append_value(group, user_name);
        }
        if (strlen(current->token) > 0) {
            struct ini_value *token = ini_value_new(CONFIG_KEY_TOKEN, (char*)current->token);
            ini_group_append_value(group, token);
        }
        if (strlen(current->session_key) > 0) {
            struct ini_value *session_key = ini_value_new(CONFIG_KEY_SESSION, (char*)current->session_key);
            ini_group_append_value(group, session_key);
        }
        if (strlen(current->url) > 0) {
            struct ini_value *url = ini_value_new(CONFIG_KEY_URL, (char*)current->url);
            ini_group_append_value(group, url);
        }

        ini_config_append_group(creds_config, group);
    }

    return creds_config;
}

static struct api_credentials *api_credentials_new(void)
{
    struct api_credentials *credentials = calloc(1, sizeof(struct api_credentials));
    if (NULL == credentials) { return NULL; }

    credentials->end_point = api_unknown;
    credentials->enabled = false;
    credentials->authenticated = false;
    return credentials;
}

static void api_credentials_disable(struct api_credentials *credentials)
{
    memset(credentials, 0x0, sizeof(struct api_credentials));
}

static void api_credentials_free(struct api_credentials *credentials)
{
    if (NULL == credentials) { return; }
    if (credentials->enabled) {
        _trace2("mem::free::credentials(%p): %s", credentials, get_api_type_label(credentials->end_point));
    }

    free(credentials);
}

extern char **environ;
static void load_environment(struct env_variables *env)
{
    if (NULL == env) { return; }

    const size_t home_var_len = strlen(HOME_VAR_NAME);
    const size_t username_var_len = strlen(USERNAME_VAR_NAME);
    const size_t config_home_var_len = strlen(XDG_CONFIG_HOME_VAR_NAME);
    const size_t data_home_var_len = strlen(XDG_DATA_HOME_VAR_NAME);
    const size_t cache_home_var_len = strlen(XDG_CACHE_HOME_VAR_NAME);
    const size_t runtime_dir_var_len = strlen(XDG_RUNTIME_DIR_VAR_NAME);

    size_t i = 0;
    while(environ[i]) {
        const char *current = environ[i];
        if (strncmp(current, HOME_VAR_NAME, home_var_len) == 0) {
            strncpy((char*)env->home, current + home_var_len + 1, HOME_PATH_MAX);
        }
        if (strncmp(current, USERNAME_VAR_NAME, username_var_len) == 0) {
            strncpy((char*)env->user_name, current + username_var_len + 1, USER_NAME_MAX);
        }
        if (strncmp(current, XDG_CONFIG_HOME_VAR_NAME, config_home_var_len) == 0) {
            strncpy((char*)env->xdg_config_home, current + config_home_var_len + 1, XDG_PATH_ELEM_MAX);
        }
        if (strncmp(current, XDG_DATA_HOME_VAR_NAME, data_home_var_len) == 0) {
            strncpy((char*)env->xdg_data_home, current + data_home_var_len + 1, XDG_PATH_ELEM_MAX);
        }
        if (strncmp(current, XDG_CACHE_HOME_VAR_NAME, cache_home_var_len) == 0) {
            strncpy((char*)env->xdg_cache_home, current + cache_home_var_len + 1, XDG_PATH_ELEM_MAX);
        }
        if (strncmp(current, XDG_RUNTIME_DIR_VAR_NAME, runtime_dir_var_len) == 0) {
            strncpy((char*)env->xdg_runtime_dir, current + runtime_dir_var_len + 1, XDG_PATH_ELEM_MAX);
        }
        i++;
    }
    if (strlen(env->user_name) > 0 && strlen(env->home) == 0) {
        snprintf((char*)env->home, HOME_PATH_MAX, TOKENIZED_DATA_DIR, HOME_DIR, env->user_name);
    }
    if (strlen(env->home) > 0) {
        if (strlen(env->xdg_data_home) == 0) {
            snprintf((char*)&env->xdg_data_home, XDG_PATH_ELEM_MAX, TOKENIZED_DATA_DIR, env->home, DATA_DIR_NAME);
        }
        if (strlen(env->xdg_config_home) == 0) {
            snprintf((char*)&env->xdg_config_home, XDG_PATH_ELEM_MAX, TOKENIZED_CONFIG_DIR, env->home, CONFIG_DIR_NAME);
        }
        if (strlen(env->xdg_cache_home) == 0) {
            snprintf((char*)&env->xdg_cache_home, XDG_PATH_ELEM_MAX, TOKENIZED_CACHE_DIR, env->home, CACHE_DIR_NAME);
        }
    }
}

static void set_cache_file(const struct configuration *config, const char *file_name)
{
    if (NULL == config) { return; }

    if (NULL == file_name) {
        file_name = "";
    }

    const int wrote = snprintf((char*)config->cache_path, FILE_PATH_MAX-3, TOKENIZED_CACHE_PATH, config->env.xdg_cache_home, config->name, file_name);
    if (wrote == 0) {
        _trace2("path::error: unable build cache path");
    }
}

static void set_cache_path(const struct configuration *config)
{
    set_cache_file(config, CACHE_FILE_NAME);
}

static void set_credentials_file(const struct configuration *config, const char *file_name)
{
    if (NULL == config) { return; }

    if (NULL == file_name) {
        file_name = "";
    }

    const int wrote = snprintf((char*)config->credentials_path, FILE_PATH_MAX, TOKENIZED_CREDENTIALS_PATH, config->env.xdg_data_home, config->name, file_name);
    if (wrote == 0) {
        _trace2("path::error: unable build credentials path");
    }
}

static void set_credentials_path(const struct configuration *config)
{
    set_credentials_file(config, CREDENTIALS_FILE_NAME);
}

static void set_config_file(const struct configuration *config, const char *file_name)
{
    if (NULL == config) { return; }

    if (NULL == file_name) {
        file_name = "config";
    }

    const int wrote = snprintf((char*)config->config_path, FILE_PATH_MAX+1, TOKENIZED_CONFIG_PATH, config->env.xdg_config_home, config->name, file_name);
    if (wrote == 0) {
        _trace2("path::error: unable build config path");
    }
}

static void set_config_path(const struct configuration *config)
{
    set_config_file(config, CONFIG_FILE_NAME);
}

static bool cleanup_pid(const char *path)
{
    if(NULL == path) { return false; }
    return (unlink(path) == 0);
}

static int load_pid_path(const struct configuration *config)
{
    if (NULL == config) { return 0; }

    return snprintf((char*)config->pid_path, FILE_PATH_MAX-3, TOKENIZED_PID_PATH, config->env.xdg_runtime_dir, config->name, PID_SUFFIX);
}

static bool load_credentials_from_ini_group (struct ini_group *group, struct api_credentials *credentials)
{
    if (NULL == credentials) { return false; }
    if (NULL == group) { return false; }
#if 0
    _trace("api::loaded:%s", group->name);
#endif

    if (strncmp(group->name->data, SERVICE_LABEL_LASTFM, strlen(SERVICE_LABEL_LASTFM)) == 0) {
        (credentials)->end_point = api_lastfm;
    } else if (strncmp(group->name->data, SERVICE_LABEL_LIBREFM, strlen(SERVICE_LABEL_LIBREFM)) == 0) {
        (credentials)->end_point = api_librefm;
    } else if (strncmp(group->name->data, SERVICE_LABEL_LISTENBRAINZ, strlen(SERVICE_LABEL_LISTENBRAINZ)) == 0) {
        (credentials)->end_point = api_listenbrainz;
    }

    const size_t count = arrlen(group->values);
    for (size_t i = 0; i < count; i++) {
        struct ini_value *setting = group->values[i];
        const string key = setting->key;
        if (NULL == key) { continue; }

        const string value = setting->value;
        if (NULL == value) { continue; }

        if (value->len == 0) { continue; }

        if (strncmp(key->data, CONFIG_KEY_ENABLED, strlen(CONFIG_KEY_ENABLED)) == 0) {
            if (strncmp(value->data, CONFIG_VALUE_TRUE, strlen(CONFIG_VALUE_TRUE)) == 0) {
                (credentials)->enabled = true;
            }
            if (strncmp(value->data, CONFIG_VALUE_ONE, strlen(CONFIG_VALUE_ONE)) == 0) {
                (credentials)->enabled = true;
            }
            // NOTE(marius): redundant, as false should be the default if nothing is present
            if (strncmp(value->data, CONFIG_VALUE_FALSE, strlen(CONFIG_VALUE_FALSE)) == 0) {
                (credentials)->enabled = false;
            }
            if (strncmp(value->data, CONFIG_VALUE_ZERO, strlen(CONFIG_VALUE_ZERO)) == 0) {
                (credentials)->enabled = false;
            }
        }
        if (strncmp(key->data, CONFIG_KEY_USER_NAME, strlen(CONFIG_KEY_USER_NAME)) == 0) {
            strncpy((credentials)->user_name, value->data, value->len + 1);
        }
        if (strncmp(key->data, CONFIG_KEY_PASSWORD, strlen(CONFIG_KEY_PASSWORD)) == 0) {
            strncpy((credentials)->password, value->data, value->len + 1);
        }
        if (strncmp(key->data, CONFIG_KEY_TOKEN, strlen(CONFIG_KEY_TOKEN)) == 0) {
            strncpy((char*)(credentials)->token, value->data, value->len + 1);
        }
        if (strncmp(key->data, CONFIG_KEY_SESSION, strlen(CONFIG_KEY_SESSION)) == 0) {
            strncpy((char*)(credentials)->session_key, value->data, value->len + 1);
        }
        switch ((credentials)->end_point) {
        case api_librefm:
        case api_listenbrainz:
            if (strncmp(key->data, CONFIG_KEY_URL, strlen(CONFIG_KEY_URL)) == 0) {
                strncpy((char*)(credentials)->url, value->data, value->len + 1);
            }
            break;
        case api_lastfm:
        case api_unknown:
        default:
            break;
        }
    }
    return true;
}

static bool write_pid(const char *path)
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

static void load_ini_from_file(struct ini_config *ini, const char* path)
{
    if (NULL == ini) { return; }
    if (NULL == path) { return; }

    FILE *file = fopen(path, "r");
    if (NULL == file) { return; }


    fseek(file, 0L, SEEK_END);
    const long file_size = ftell(file);

    if (file_size <= 0) { goto _exit; }
    rewind (file);

    char *buffer = get_zero_string((size_t)file_size);
    if (NULL == buffer) { goto _exit; }
    if (1 != fread(buffer, (size_t)file_size, 1, file)) {
        _warn("config::error: unable to read file %s", path);
        goto _failure;
    }

    if (ini_parse(buffer, (size_t)file_size, ini) < 0) {
        _error("config::error: failed to parse file %s", path);
    }

_failure:
    string_free(buffer);
_exit:
    fclose(file);
}

static bool load_config_from_file(struct configuration *config, const char* path)
{
    if (NULL == config) { return false; }
    if (NULL == path) { return false; }
    config->ignore_players_count = 0;

    struct ini_config ini = {0};
    load_ini_from_file(&ini, path);
    const size_t group_count = arrlen(ini.groups);
    for (size_t i = 0; i < group_count; i++) {
        const struct ini_group *group = ini.groups[i];
        if (NULL == group->name) { continue; }
        if (strncmp(group->name->data, DEFAULT_GROUP_NAME, group->name->len) != 0) {
            break;
        }
        const size_t value_count = arrlen(group->values);
        for (size_t j = 0; j < value_count; j++) {
            const struct ini_value *val = group->values[j];
            if (strncmp(val->key->data, CONFIG_KEY_IGNORE, val->key->len) != 0) {
                break;
            }
            const short cnt = config->ignore_players_count;
            _trace("config::ignore_player[%d]: %s", cnt, val->value->data);
            memcpy((char*)config->ignore_players[cnt], val->value->data, val->value->len);
            config->ignore_players_count++;
        }
    }
    ini_config_clean(&ini);
    return true;
}

static bool load_credentials_from_file(struct configuration *config, const char* path)
{
    if (NULL == config) { return false; }
    if (NULL == path) { return false; }

    struct ini_config ini = {0};
    load_ini_from_file(&ini, path);
    const size_t count = arrlen(ini.groups);
    for (size_t i = 0; i < count; i++) {
        struct ini_group *group = ini.groups[i];

        bool found_matching_creds = false;
        const size_t credentials_count = arrlen(config->credentials);
        struct api_credentials *creds = NULL;
        for (size_t j = 0; j < credentials_count; j++) {
            creds = config->credentials[j];
            const char *api = get_api_type_group(creds->end_point);
            if (NULL == api) {
#if 0
        _warn("ini::loaded_group: %s, searching for %s", group->name->data, api);
#endif
                continue;
            }
            if (strncmp(api, group->name->data, group->name->len) == 0) {
            //if (_grrrs_cmp_cstr(group->name, api) == 0) {
                // same group
                found_matching_creds = true;
                break;
            }
        }

        if (!found_matching_creds) { creds = api_credentials_new(); }

        if (!load_credentials_from_ini_group(group, creds)) {
            _warn("ini::invalid_config[%s]: not loading values", group->name->data);
            api_credentials_free(creds);
        } else {
            arrput(config->credentials, creds);
        }
    }
    ini_config_clean(&ini);
    return true;
}

static void load_config (struct configuration *config)
{
    set_config_path(config);
    if (load_config_from_file(config, config->config_path)) {
        _debug("main::loading_config: ok");
    } else {
        _warn("main::loading_config: failed");
    }
}

static void load_credentials (struct configuration *config)
{
    set_credentials_path(config);

    // Cleanup
    size_t count = 0;
    if (NULL != config->credentials) {
        // reset configuration
        count = arrlen(config->credentials);
        for (int j = (int)count - 1; j >= 0; j--) {
            if (NULL != config->credentials[j]) {
                api_credentials_free(config->credentials[j]);
                (void)arrpop(config->credentials);
                config->credentials[j] = NULL;
            }
        }
        assert(arrlen(config->credentials) == 0);
    }

    // Load
    if (load_credentials_from_file(config, config->credentials_path)) {
        _debug("main::loading_credentials: ok");
    } else {
        _warn("main::loading_credentials: failed");
    }

    if (NULL == config->credentials) { return; }

    // load
    count = arrlen(config->credentials);
    for(size_t i = 0; i < count; i++) {
        struct api_credentials *cur = config->credentials[i];
        const char *api_key = api_get_application_key(cur->end_point);
        memcpy((char*)cur->api_key, api_key, min(MAX_SECRET_LENGTH, strlen(api_key)));
        const char *api_secret = api_get_application_secret(cur->end_point);
        memcpy((char*)cur->secret, api_secret, min(MAX_SECRET_LENGTH, strlen(api_secret)));
        if (strlen(cur->api_key) == 0) {
            _warn("scrobbler::invalid_service[%s]: missing API key", get_api_type_label(cur->end_point));
            cur->enabled = false;
        }
        if (strlen(cur->secret) == 0) {
            _warn("scrobbler::invalid_service[%s]: missing API secret key", get_api_type_label(cur->end_point));
            cur->enabled = false;
        }
    }
}

static void configuration_clean(struct configuration *config)
{
    if (NULL == config) { return; }
    const size_t count = arrlen(config->credentials);
    _trace2("mem::free::configuration(%u)", count);
    for (int i = (int)count - 1; i >= 0; i--) {
        if (NULL != config->credentials[i]) {
            (void)arrpop(config->credentials);
            api_credentials_free(config->credentials[i]);
            config->credentials[i] = NULL;
        }
    }
    assert(arrlen(config->credentials) == 0);
    arrfree(config->credentials);
    if (config->wrote_pid) {
        _trace("main::cleanup_pid: %s", config->pid_path);
        cleanup_pid(config->pid_path);
    }
}

static void print_application_config(const struct configuration *config)
{
    printf("app::name %s\n", config->name);
    printf("app::user %s\n", config->env.user_name);
    printf("app::home_folder %s\n", config->env.home);
    printf("app::config_folder %s\n", config->env.xdg_config_home);
    printf("app::data_folder %s\n", config->env.xdg_data_home);
    printf("app::cache_folder %s\n", config->env.xdg_cache_home);
    printf("app::runtime_dir %s\n", config->env.xdg_runtime_dir);

    const size_t credentials_count = arrlen(config->credentials);
    printf("app::loaded_credentials_count %zu\n", (size_t)credentials_count);

    if (credentials_count == 0) { return; }
    for (size_t i = 0 ; i < credentials_count; i++) {
        const struct api_credentials *cur = config->credentials[i];
        printf("app::credentials[%zu]:%s\n", (size_t)i, get_api_type_label(cur->end_point));
        printf("\tenabled = %s\n", cur->enabled ? "true" : "false" );
        printf("\tauthenticated = %s\n", cur->authenticated ? "true" : "false");
        if (strlen(cur->url) > 0) {
            printf("\turl = %s\n", cur->url);
        }
        if (strlen(cur->user_name) > 0) {
            printf("\tusername = %s\n", cur->user_name);
        }
        if (strlen(cur->password) > 0) {
            printf("\tpassword = %s\n", cur->password);
        }
        if (strlen(cur->token) > 0) {
            printf("\ttoken = %s\n", cur->token);
        }
        if (strlen(cur->session_key)) {
            printf("\tsession_key = %s\n", cur->session_key);
        }
    }
}

bool load_configuration(struct configuration *config, const char *name)
{
    if (NULL == config) { return false; }
    if (NULL != name) {
        memcpy((char*)config->name, name, min(USER_NAME_MAX, strlen(name)));
    }

    if (!config->env_loaded) {
        load_environment(&config->env);
        config->env_loaded = true;
    }

    set_config_path(config);
    set_credentials_path(config);
    set_cache_path(config);

    load_config(config);

    load_credentials(config);

    return true;
}

bool configuration_folder_exists(const char *path)
{
    if (NULL == path) { return false; }

    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

bool configuration_folder_create(const char *path)
{
    const char *err_msg = NULL;

    bool status = (mkdir(path, S_IRWXU) == 0);
    if (!status) {
        switch(errno) {
            case EACCES:
                err_msg = "permission denied when writing folder";
                break;
            case EEXIST:
                err_msg = "the folder already exists";
                status = true;
                break;
            case ELOOP:
                err_msg = "unable to resolve folder path";
                break;
            case EMLINK:
                err_msg = "link count exceeded";
                break;
            case ENAMETOOLONG:
                err_msg = "path length is too long";
                break;
            case ENOENT:
                err_msg = "unable to resolve folder path, parent is missing";
                break;
            case ENOSPC:
                err_msg = "not enough space to create folder";
                break;
            case ENOTDIR:
                err_msg = "parent is not a folder";
                break;
            case EROFS:
                err_msg = "parent file-system is read-only";
                break;
            default:
                err_msg = "unknown error";
        }
        _trace("credentials::folder_create_error: %s", err_msg);
    }
    return status;
}

static int write_credentials_file(struct configuration *config) {
    int status = -1;
    struct ini_config *to_write = NULL;

    char file_path[FILE_PATH_MAX+1] = {0};
    strncpy(file_path, config->credentials_path, FILE_PATH_MAX+1);
    char *folder_path = dirname(file_path);
    if (!configuration_folder_exists(folder_path) && !configuration_folder_create(folder_path)) {
        _error("main::credentials: unable to create data folder %s", folder_path);
        goto _return;
    }

    const size_t count = arrlen(config->credentials);
    to_write = get_ini_from_credentials(config->credentials, count);
#if 0
    print_application_config(config);
    print_ini(to_write);
#endif

    _debug("saving::credentials[%u]: %s", count, config->credentials_path);
    FILE *file = fopen(config->credentials_path, "w+");
    if (NULL == file) {
        _warn("saving::credentials:failed: %s", config->credentials_path);
        goto _return;
    }
    status = write_ini_file(to_write, file);
    fclose(file);

_return:
    if (NULL != to_write) { ini_config_free(to_write); }

    return status;
}

#endif // MPRIS_SCROBBLER_CONFIGURATION_H
