#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <basedir.h>
#include <basedir_fs.h>

#define array_count(a) (sizeof(a)/sizeof 0[a])

#define SCROBBLE_BUFF_LEN 2
#define SLEEP_USECS 100000

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
    unknown = -(1 << 1),
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
    extern log_level _log_level;

#ifndef DEBUG
    if (level == debug || level == tracing) { return 0; }
#endif
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

void handle_sigalrm(int signal) {
    if (signal != SIGALRM) {
        _log(warning, "main::unexpected_signal: %d", signal);
    }
}

void free_credentials(lastfm_credentials *credentials) {
    if (NULL != credentials->user_name) free(credentials->user_name);
    if (NULL != credentials->password) free(credentials->password);
}

bool load_credentials(lastfm_credentials* credentials)
{
    if (NULL == credentials) { goto _error; }
    const char *path = CREDENTIALS_PATH;

    xdgHandle handle;
    if(!xdgInitHandle(&handle)) {
        goto _error;
    }

    FILE *config = xdgConfigOpen(path, "r", &handle);

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

    xdgWipeHandle(&handle);
    _log(debug, "main::load_credentials: %s:%s", user_name, pass_label);
    return true;
_error:
    xdgWipeHandle(&handle);
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
    //int secs = 1;
    //if (usecs > 10000) {
    //    secs = usecs/10000;
    //}
    //alarm(secs);
    usleep(usecs);
    sigsuspend(&mask);
}

