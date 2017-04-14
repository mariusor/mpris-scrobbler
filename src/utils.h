#include <string.h>

#define APPLICATION_NAME "mpris-scrobbler"
#define CREDENTIALS_PATH APPLICATION_NAME "/credentials"

#define LOG_ERROR_LABEL "ERROR"
#define LOG_WARNING_LABEL "WARNING"
#define LOG_DEBUG_LABEL "DEBUG"
#define LOG_INFO_LABEL "INFO"

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

    size_t p_len = l_len + 1;
    char* preffix = (char *)malloc(p_len + 1);
    snprintf(preffix, p_len + 1, "%7s ", label);

    char* suffix = "\n";
    size_t s_len = strlen(suffix);
    size_t f_len = strlen(format);
    size_t full_len = p_len + f_len + s_len;

    char* log_format = (char *)malloc(full_len + 1);

    strncpy(log_format, preffix, p_len);
    free(preffix);

    strncpy(log_format + p_len, format, f_len);
    strncpy(log_format + p_len + f_len, suffix, s_len);

    int result = vfprintf(stderr, log_format, args);
    free(log_format);
    va_end(args);

    return result;
}

