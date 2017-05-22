#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define array_count(a) (sizeof(a)/sizeof 0[a])

#define SCROBBLE_BUFF_LEN 2
#define SLEEP_USECS 20000

#define APPLICATION_NAME "mpris-scrobbler"
#define CREDENTIALS_PATH APPLICATION_NAME "/credentials"

#define LOG_ERROR_LABEL "ERROR"
#define LOG_WARNING_LABEL "WARNING"
#define LOG_DEBUG_LABEL "DEBUG"
#define LOG_TRACING_LABEL "TRACING"
#define LOG_INFO_LABEL "INFO"

typedef struct _credentials {
    char* user_name;
    char* password;
} _credentials;

typedef enum log_levels
{
    unknown = (1 << 0),
    tracing = (1 << 1),
    debug   = (1 << 2),
    info    = (1 << 3),
    warning = (1 << 4),
    error   = (1 << 5)
} log_level;

typedef struct lastfm_credentials {
    char* user_name;
    char* password;
} lastfm_credentials;

const char* get_log_level (log_level l) {
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

int _log(log_level level, const char* format, ...)
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
    char* preffix = (char *)calloc(1, p_len + 1);
    snprintf(preffix, p_len + 1, "%-7s ", label);

    char* suffix = "\n";
    size_t s_len = strlen(suffix);
    size_t f_len = strlen(format);
    size_t full_len = p_len + f_len + s_len;

    char* log_format = (char *)calloc(1, full_len + 1);

    strncpy(log_format, preffix, p_len);
    free(preffix);

    strncpy(log_format + p_len, format, f_len);
    strncpy(log_format + p_len + f_len, suffix, s_len);

    int result = vfprintf(stderr, log_format, args);
    free(log_format);
    va_end(args);

    return result;
}

void free_credentials(lastfm_credentials *credentials) {
    if (NULL != credentials->user_name) free(credentials->user_name);
    if (NULL != credentials->password) free(credentials->password);
}

void zero_string(char** incoming, size_t length)
{
    if (NULL == incoming) { return; }
    size_t length_with_null = (length + 1) * sizeof(char);
    memset(*incoming, 0, length_with_null);
}

char* get_zero_string(size_t length)
{
    size_t length_with_null = (length + 1) * sizeof(char);
    char* result = (char*)calloc(1, length_with_null);
    if (NULL == result) {
        _log(error & tracing, "mem::could_not_allocate %lu bytes string", length_with_null);
#if 0
    } else {
        _log(tracing, "mem::allocated %lu bytes string", length_with_null);
#endif
    }

    return result;
}

#define CONFIG_DIR_NAME ".config"
#define TOKENIZED_HOME_USER_PATH "%s/%s/%s"
#define TOKENIZED_XDG_CONFIG_PATH "%s/%s"
#define HOME_VAR_NAME "HOME"
#define USERNAME_VAR_NAME "USER"
#define XDG_CONFIG_HOME_VAR_NAME "XDG_CONFIG_HOME"
#define XDG_DATA_HOME_VAR_NAME "XDG_DATA_HOME"

FILE *get_config_file(const char* path, const char* mode)
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

bool load_credentials(lastfm_credentials* credentials)
{
    if (NULL == credentials) { goto _error; }
    const char *path = CREDENTIALS_PATH;

    FILE *config = get_config_file(path, "r");

    if (!config) { goto _error; }

    char user_name[256];
    char password[256];
    size_t it = 0;
    char current;
    while (!feof(config)) {
        current = fgetc(config);
        user_name[it++] = current;
        if (current == ':') { break; }
    }
    user_name[it-1] = '\0';
    it = 0;
    while (!feof(config)) {
        current = fgetc(config);
        password[it++] = current;
        if (current == '\n') { break; }
    }
    password[it-1] = '\0';
    fclose(config);

    size_t u_len = strlen(user_name);
    size_t p_len = strlen(password);

    if (u_len  == 0 || p_len == 0) { goto _error; }

    credentials->user_name = (char *)calloc(1, u_len+1);
    if (NULL == credentials->user_name) { free_credentials(credentials); goto _error; }

    credentials->password = (char *)calloc(1, p_len+1);
    if (!credentials->password) { free_credentials(credentials); goto _error; }

    strncpy(credentials->user_name, user_name, u_len);
    strncpy(credentials->password, password, p_len);

    size_t pl_len = strlen(credentials->password);
    char pass_label[256];

    for (size_t i = 0; i < pl_len; i++) {
        pass_label[i] = '*';
    }
    pass_label[pl_len] = '\0';

    _log(debug, "main::load_credentials: %s:%s", user_name, pass_label);
    return true;
_error:
    _log(error, "main::load_credentials: failed");
    return false;
}

void sighandler(int signum)
{
    extern bool done;
    extern bool reload;
    const char* signal_name = "UNKNOWN";
    //sigset_t pending;

    switch (signum) {
        case SIGHUP:
            signal_name = "SIGHUP";
            break;
        case SIGUSR1:
            signal_name = "SIGUSR1";
            break;
        case SIGINT:
            signal_name = "SIGINT";
            break;
        case SIGTERM:
            signal_name = "SIGTERM";
            break;

    }
    _log(info, "main::signal_received: %s", signal_name);
    if (signum == SIGHUP) { reload = true; }
    if (signum == SIGINT || signum == SIGTERM) { done = true; }
}

void handle_sigalrm(int signal) {
    if (signal != SIGALRM) {
        _log(warning, "main::unexpected_signal: %d", signal);
    }
}

void do_sleep(useconds_t usecs)
{
    struct sigaction sa;
    sigset_t mask;

    sa.sa_handler = &handle_sigalrm; // Intercept and ignore SIGALRM
    sa.sa_flags = SA_RESETHAND; // Remove the handler after first signal
    sigfillset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    // Get the current signal mask
    sigprocmask(0, NULL, &mask);

    // Unblock SIGALRM
    sigdelset(&mask, SIGALRM);

    // Wait with this mask
    ualarm(usecs, 0);
    sigsuspend(&mask);
}
