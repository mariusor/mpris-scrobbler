/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_CONFIGURATION_H
#define MPRIS_SCROBBLER_CONFIGURATION_H

#include <sys/stat.h>

#ifndef APPLICATION_NAME
#define APPLICATION_NAME            "mpris-scrobbler"
#endif

#define PID_SUFFIX                  ".pid"
#define CREDENTIALS_FILE_NAME       "credentials"
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

struct ini_config *get_ini_from_credentials(struct api_credentials *credentials[], size_t length)
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

        if (NULL != current->user_name && strlen(current->user_name) > 0) {
            struct ini_value *user_name = ini_value_new(CONFIG_KEY_USER_NAME, (char*)current->user_name);
            ini_group_append_value(group, user_name);
        }
        if (NULL != current->token && strlen(current->token) > 0) {
            struct ini_value *token = ini_value_new(CONFIG_KEY_TOKEN, (char*)current->token);
            ini_group_append_value(group, token);
        }
        if (NULL != current->session_key && strlen(current->session_key) > 0) {
            struct ini_value *session_key = ini_value_new(CONFIG_KEY_SESSION, (char*)current->session_key);
            ini_group_append_value(group, session_key);
        }
        if (NULL != current->url && strlen(current->url) > 0) {
            struct ini_value *url = ini_value_new(CONFIG_KEY_URL, (char*)current->url);
            ini_group_append_value(group, url);
        }

        ini_config_append_group(creds_config, group);
    }

    return creds_config;
}

struct api_credentials *api_credentials_new(void)
{
    struct api_credentials *credentials = malloc(sizeof(struct api_credentials));
    if (NULL == credentials) { return NULL; }

    credentials->end_point = api_unknown;
    credentials->enabled = false;
    credentials->authenticated = false;
    credentials->token = get_zero_string(MAX_PROPERTY_LENGTH);
    credentials->session_key = get_zero_string(MAX_PROPERTY_LENGTH);
    credentials->user_name = get_zero_string(MAX_PROPERTY_LENGTH);
    credentials->password = get_zero_string(MAX_PROPERTY_LENGTH);
    credentials->url = get_zero_string(MAX_URL_LENGTH);

    return credentials;
}

void api_credentials_disable(struct api_credentials *credentials)
{
    if (NULL != credentials->user_name) {
        memset(credentials->user_name, 0x0, MAX_PROPERTY_LENGTH);
    }
    if (NULL != credentials->password) {
        memset(credentials->password, 0x0, MAX_PROPERTY_LENGTH);
    }
    if (NULL != credentials->token) {
        memset((char*)credentials->token, 0x0, MAX_PROPERTY_LENGTH);
    }
    if (NULL != credentials->session_key) {
        memset((char*)credentials->session_key, 0x0, MAX_PROPERTY_LENGTH);
    }
    if (NULL != credentials->url) {
        memset((char*)credentials->url, 0x0, MAX_PROPERTY_LENGTH);
    }

    credentials->enabled = false;
}

void api_credentials_free(struct api_credentials *credentials)
{
    if (NULL == credentials) { return; }
    if (credentials->enabled) {
        _trace2("mem::free::credentials(%p): %s", credentials, get_api_type_label(credentials->end_point));
    }
    if (NULL != credentials->user_name) { string_free(credentials->user_name); }
    if (NULL != credentials->password)  { string_free(credentials->password); }
    if (NULL != credentials->token)  { string_free((char*)credentials->token); }
    if (NULL != credentials->session_key)  { string_free((char*)credentials->session_key); }
    if (NULL != credentials->url)  { string_free((char*)credentials->url); }
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
            strncpy((char*)env->home, current + home_var_len + 1, home_len);
        }
        if (strncmp(current, USERNAME_VAR_NAME, username_var_len) == 0) {
            username_len = current_len - username_var_len;
            strncpy((char*)env->user_name, current + username_var_len + 1, username_len);
        }
        if (strncmp(current, XDG_CONFIG_HOME_VAR_NAME, config_home_var_len) == 0) {
            config_home_len = current_len - config_home_var_len;
            strncpy((char*)env->xdg_config_home, current + config_home_var_len + 1, config_home_len);
        }
        if (strncmp(current, XDG_DATA_HOME_VAR_NAME, data_home_var_len) == 0) {
            data_home_len = current_len - data_home_var_len;
            strncpy((char*)env->xdg_data_home, current + data_home_var_len + 1, data_home_len);
        }
        if (strncmp(current, XDG_CACHE_HOME_VAR_NAME, cache_home_var_len) == 0) {
            cache_home_len = current_len - cache_home_var_len;
            strncpy((char*)env->xdg_cache_home, current + cache_home_var_len + 1, cache_home_len);
        }
        if (strncmp(current, XDG_RUNTIME_DIR_VAR_NAME, runtime_dir_var_len) == 0) {
            runtime_dir_len = current_len - runtime_dir_var_len;
            strncpy((char*)env->xdg_runtime_dir, current + runtime_dir_var_len + 1, runtime_dir_len);
        }
        i++;
    }
    if (strlen(env->user_name) > 0 && strlen(env->home) == 0) {
        snprintf((char*)env->home, HOME_LENGTH, TOKENIZED_DATA_DIR, HOME_DIR, env->user_name);
    }
    if (strlen(env->home) > 0) {
        if (strlen(env->xdg_data_home) == 0) {
            snprintf((char*)&env->xdg_data_home, MAX_PROPERTY_LENGTH, TOKENIZED_DATA_DIR, env->home, DATA_DIR_NAME);
        }
        if (strlen(env->xdg_config_home) == 0) {
            snprintf((char*)&env->xdg_config_home, MAX_PROPERTY_LENGTH, TOKENIZED_CONFIG_DIR, env->home, CONFIG_DIR_NAME);
        }
        if (strlen(env->xdg_cache_home) == 0) {
            snprintf((char*)&env->xdg_cache_home, MAX_PROPERTY_LENGTH, TOKENIZED_CACHE_DIR, env->home, CACHE_DIR_NAME);
        }
    }
}

static char *get_credentials_path(struct configuration *config, const char *file_name)
{
    if (NULL == config) { return NULL; }

    if (NULL == file_name) {
        file_name = "";
    }

    size_t name_len = strlen(config->name);
    size_t data_home_len = strlen(config->env.xdg_data_home);
    size_t cred_len = strlen(file_name);
    size_t path_len = name_len + data_home_len + cred_len + 2;

    char *path = get_zero_string(path_len);
    if (NULL == path) { return NULL; }

    snprintf(path, path_len + 1, TOKENIZED_CREDENTIALS_PATH, config->env.xdg_data_home, config->name, file_name);
    return path;
}

char *get_credentials_file(struct configuration *config)
{
    return get_credentials_path(config, CREDENTIALS_FILE_NAME);
}

static char *get_config_path(struct configuration *config, const char *file_name)
{
    if (NULL == config) { return NULL; }

    if (NULL == file_name) {
        file_name = "config";
    }

    size_t name_len = strlen(config->name);
    size_t config_home_len = strlen(config->env.xdg_config_home);
    size_t cred_len = strlen(file_name);
    size_t path_len = name_len + config_home_len + cred_len + 2;

    char *path = get_zero_string(path_len);
    if (NULL == path) { return NULL; }

    snprintf(path, path_len + 1, TOKENIZED_CONFIG_PATH, config->env.xdg_config_home, config->name, file_name);
    return path;
}

char *get_config_file(struct configuration *config)
{
    return get_config_path(config, CONFIG_FILE_NAME);
}

bool cleanup_pid(const char *path)
{
    if(NULL == path) { return false; }
    return (unlink(path) == 0);
}

int load_pid_path(struct configuration *config)
{
    if (NULL == config) { return 0; }

    size_t name_len = strlen(config->name);
    size_t ext_len = strlen(PID_SUFFIX);
    size_t runtime_dir_len = strlen(config->env.xdg_runtime_dir);
    size_t path_len = name_len + runtime_dir_len + ext_len + 2;

    path_len = snprintf((char*)config->pid_path, path_len, TOKENIZED_PID_PATH, config->env.xdg_runtime_dir, config->name, PID_SUFFIX);

    return path_len;
}

bool load_credentials_from_ini_group (struct ini_group *group, struct api_credentials *credentials)
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

    int count = arrlen(group->values);
    for (int i = 0; i < count; i++) {
        struct ini_value *setting = group->values[i];
        string key = setting->key;
        if (NULL == key) { continue; }

        string value = setting->value;
        if (NULL == value) { continue; }

        if (value->len == 0) { continue; }

        if (strncmp(key->data, CONFIG_KEY_ENABLED, strlen(CONFIG_KEY_ENABLED)) == 0) {
            (credentials)->enabled = (strncmp(value->data, CONFIG_VALUE_FALSE, strlen(CONFIG_VALUE_FALSE)) && strncmp(value->data, CONFIG_VALUE_ZERO, strlen(CONFIG_VALUE_ZERO)));
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

void load_ini_from_file(struct ini_config *ini, const char* path)
{
    if (NULL == ini) { return; }
    if (NULL == path) { return; }

    FILE *file = fopen(path, "r");
    if (NULL == file) { return; }

    long file_size;

    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);

    if (file_size <= 0) { return; }
    rewind (file);

    char *buffer = get_zero_string(file_size);
    if (1 != fread(buffer, file_size, 1, file)) {
        _warn("config::error: unable to read file %s", path);
        fclose(file);
        return;
    }
    fclose(file);

    if (ini_parse(buffer, file_size, ini) < 0) {
        _error("config::error: failed to parse file %s", path);
    }
    string_free(buffer);
}

void load_config_from_file(struct configuration *config, const char* path)
{
    if (NULL == config) { return; }
    if (NULL == path) { return; }

    struct ini_config ini = {0};
    load_ini_from_file(&ini, path);
    int group_count = arrlen(ini.groups);
    for (int i = 0; i < group_count; i++) {
        struct ini_group *group = ini.groups[i];
        if (strncmp(group->name->data, DEFAULT_GROUP_NAME, group->name->len) != 0) {
            break;
        }
        int value_count = arrlen(group->values);
        for (int j = 0; j < value_count; j++) {
            struct ini_value *val = group->values[j];
            if (strncmp(val->key->data, CONFIG_KEY_IGNORE, val->key->len) != 0) {
                break;
            }
            int cnt = config->ignore_players_count;
            memcpy((char*)config->ignore_players[cnt],val->value->data, val->value->len);
            _trace("config::loaded_ignored_player: %s", config->ignore_players[cnt]);
            config->ignore_players_count++;
        }
    }
    ini_config_clean(&ini);
}

void load_credentials_from_file(struct configuration *config, const char* path)
{
    if (NULL == config) { return; }
    if (NULL == path) { return; }

    struct ini_config ini = {0};
    load_ini_from_file(&ini, path);
    int count = arrlen(ini.groups);
    for (int i = 0; i < count; i++) {
        struct ini_group *group = ini.groups[i];

        bool found_matching_creds = false;
        int credentials_count = arrlen(config->credentials);
        struct api_credentials *creds = NULL;
        for (int j = 0; j < credentials_count; j++) {
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
}

void load_config (struct configuration *config)
{
    char *path = get_config_file(config);
    if (NULL != path) {
        load_config_from_file(config, path);
        _debug("main::loading_config: ok");
    } else {
        _warn("main::loading_config: failed");
    }
    string_free(path);
}

void load_credentials (struct configuration *config)
{
    char *credentials_file = get_credentials_file(config);
    if (NULL != credentials_file) {
        load_credentials_from_file(config, credentials_file);
        _debug("main::loading_credentials: ok");
    } else {
        _warn("main::loading_credentials: failed");
    }
    string_free(credentials_file);
}

void configuration_clean(struct configuration *config)
{
    if (NULL == config) { return; }
    int count = arrlen(config->credentials);
    _trace2("mem::free::configuration(%u)", count);
    if (count > 0) {
        for (int i = count - 1; i >= 0; i--) {
            if (NULL != config->credentials[i]) {
                (void)arrpop(config->credentials);
                api_credentials_free(config->credentials[i]);
                config->credentials[i] = NULL;
            }
        }
    }
    assert(arrlen(config->credentials) == 0);
    arrfree(config->credentials);
    if (config->wrote_pid) {
        _trace("main::cleanup_pid: %s", config->pid_path);
        cleanup_pid(config->pid_path);
    }

}

void print_application_config(struct configuration *config)
{
    printf("app::name %s\n", config->name);
    printf("app::user %s\n", config->env.user_name);
    printf("app::home_folder %s\n", config->env.home);
    printf("app::config_folder %s\n", config->env.xdg_config_home);
    printf("app::data_folder %s\n", config->env.xdg_data_home);
    printf("app::cache_folder %s\n", config->env.xdg_cache_home);
    printf("app::runtime_dir %s\n", config->env.xdg_runtime_dir);

    int credentials_count = arrlen(config->credentials);
    printf("app::loaded_credentials_count %zu\n", (size_t)credentials_count);

    if (credentials_count == 0) { return; }
    for (int i = 0 ; i < credentials_count; i++) {
        struct api_credentials *cur = config->credentials[i];
        printf("app::credentials[%zu]:%s\n", (size_t)i, get_api_type_label(cur->end_point));
        printf("\tenabled = %s\n", cur->enabled ? "true" : "false" );
        printf("\tauthenticated = %s\n", cur->authenticated ? "true" : "false");
        if (NULL != cur->url) {
            printf("\turl = %s\n", cur->url);
        }
        if (NULL != cur->user_name) {
            printf("\tusername = %s\n", cur->user_name);
        }
        if (NULL != cur->password) {
            printf("\tpassword = %s\n", cur->password);
        }
        if (NULL != cur->token) {
            printf("\ttoken = %s\n", cur->token);
        }
        if (NULL != cur->session_key) {
            printf("\tsession_key = %s\n", cur->session_key);
        }
    }
}

const char *api_get_application_secret(enum api_type);
const char *api_get_application_key(enum api_type);
bool load_configuration(struct configuration *config, const char *name)
{
    if (NULL == config) { return false; }
    if (NULL != name) {
        strncpy((char*)config->name, name, MAX_PROPERTY_LENGTH - 1);
    }

    if (!config->env_loaded) {
        load_environment(&config->env);
        config->env_loaded = true;
    }

    int count = 0;
    if (NULL != config->credentials) {
        // reset configuration
        count = arrlen(config->credentials);
        for (int j = count - 1; j >= 0; j--) {
            if (NULL != config->credentials[j]) {
                api_credentials_free(config->credentials[j]);
                (void)arrpop(config->credentials);
                config->credentials[j] = NULL;
            }
        }
        assert(arrlen(config->credentials) == 0);
    }

    load_credentials(config);
    load_config(config);

    if (NULL != config->credentials) {
        count = arrlen(config->credentials);
        for(int i = 0; i < count; i++) {
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
    }
    load_pid_path(config);

    _trace("main::writing_pid: %s", config->pid_path);
    config->wrote_pid = write_pid(config->pid_path);
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
    const char *err_msg = NULL;

    bool status = (mkdir(path, S_IRWXU) == 0);
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
    struct ini_config *to_write = NULL;

    char *folder_path = get_credentials_path(config, NULL);
    if (!credentials_folder_exists(folder_path) && !credentials_folder_create(folder_path)) {
        _error("main::credentials: Unable to create data folder %s", folder_path);
        goto _return;
    }

    int count = arrlen(config->credentials);
    to_write = get_ini_from_credentials(config->credentials, count);
    file_path = get_credentials_file(config);
#if 0
    print_application_config(config);
    print_ini(to_write);
#endif

    if (NULL != file_path) {
        _debug("saving::credentials[%u]: %s", count, file_path);
        FILE *file = fopen(file_path, "w+");
        if (NULL == file) {
            _warn("saving::credentials:failed: %s", file_path);
            goto _return;
        }
        status = write_ini_file(to_write, file);
        fclose(file);
    }

_return:
    if (NULL != to_write) { ini_config_free(to_write); }
    if (NULL != file_path) { string_free(file_path); }
    if (NULL != folder_path) { string_free(folder_path); }

    return status;
}

#endif // MPRIS_SCROBBLER_CONFIGURATION_H
