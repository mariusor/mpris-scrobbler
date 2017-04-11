#include <string.h>
#include <basedir.h>
#include <basedir_fs.h>

#define APPLICATION_NAME "mpris-scrobbler"
#define CREDENTIALS_PATH APPLICATION_NAME "/credentials"

#define LOG_ERROR_LABEL "error"
#define LOG_WARNING_LABEL "warning"
#define LOG_DEBUG_LABEL "debug"
#define LOG_INFO_LABEL "info"

typedef enum log_levels
{
    unknown,
    error,
    warning,
    debug,
    info
} log_level;

const char* get_log_level (log_level l) {
    switch (l) {
        case (error):
            return LOG_ERROR_LABEL;
        case (warning):
            return LOG_WARNING_LABEL;
        case (debug):
            return LOG_DEBUG_LABEL;
        case (info):
        default:
            return LOG_INFO_LABEL;
    }
}

int _log(log_level level, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    const char *label = get_log_level(level);
    size_t l_len = strlen(LOG_WARNING_LABEL);

    size_t p_len = l_len + 3;
    char* preffix = (char *)malloc(p_len + 1);
    snprintf(preffix, p_len + 1, "[%7s] ", label);

    char* suffix = "\n";
    size_t s_len = strlen(suffix);
    size_t f_len = strlen(format);
    size_t full_len = p_len + f_len + s_len;

    char* log_format = (char *)malloc(full_len + 1);

    strncpy(log_format, preffix, p_len);
    strncpy(log_format + p_len, format, f_len);
    strncpy(log_format + p_len + f_len, suffix, s_len);

    int result = vfprintf(stderr, log_format, args);

    free(log_format);
    free(preffix);

    return result;
}

lastfm_credentials* load_credentials()
{
    const char *path = CREDENTIALS_PATH;

    xdgHandle handle;
    if(!xdgInitHandle(&handle)) {
        return NULL;
    }

    FILE *config = xdgConfigOpen(path, "r", &handle);
    if (!config) {
        return NULL;
    }

    char user_name[256];
    char password[256];
    size_t it = 0;
    char current;
    while (!feof(config)) {
        current = fgetc(config);
        user_name[it++] = current;
        if (current == ':') {
            break;
        }
    }
    user_name[it-1] = '\0';
    it = 0;
    while (!feof(config)) {
        current = fgetc(config);
        password[it++] = current;
        if (current == '\n') {
            break;
        }
    }
    password[it-1] = '\0';
    size_t u_len = strlen(user_name);
    size_t p_len = strlen(password);
    lastfm_credentials *credentials = (lastfm_credentials *)malloc(sizeof(lastfm_credentials));
    if (!credentials) { return NULL; }

    credentials->user_name = (char *)calloc(1, u_len+1);
    if (!credentials->user_name) { return free_credentials(credentials); }

    credentials->password = (char *)calloc(1, p_len+1);
    if (!credentials->password) { return free_credentials(credentials); }

    strncpy(credentials->user_name, user_name, u_len);
    strncpy(credentials->password, password, p_len);

    // Always remember to close the file.
    fclose(config);
    xdgWipeHandle(&handle);

    size_t pl_len = strlen(credentials->password);
    char* pass_label = (char *)malloc(pl_len + 1);
    for (size_t i = 0; i < pl_len; i++) {
        pass_label[i] = '*';
    }
    _log(info, "Loaded credentials: %s:%s", credentials->user_name, pass_label);
    return credentials;
}
