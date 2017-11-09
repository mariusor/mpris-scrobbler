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

enum log_levels _log_level;

#define array_count(a) (sizeof(a)/sizeof 0[a])
#define imax(a, b) ((a > b) ? b : a)

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

static bool level_is(unsigned incoming, enum log_levels level)
{
    return ((incoming & level) == level);
}

static const char* get_log_level (enum log_levels l)
{
    if (level_is(l, tracing)) { return LOG_TRACING_LABEL; }
    if (level_is(l, debug)) { return LOG_DEBUG_LABEL; }
    if (level_is(l, info)) { return LOG_INFO_LABEL; }
    if (level_is(l, warning)) { return LOG_WARNING_LABEL; }
    if (level_is(l, error)) { return LOG_ERROR_LABEL; }
    return LOG_TRACING_LABEL;
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
        case(unknown):
        default:
            api_label = "unknown";
            break;
    }

    return api_label;
}

void zero_string(char** incoming, size_t length)
{
    if (NULL == incoming) { return; }
    size_t length_with_null = (length + 1) * sizeof(char);
    memset(*incoming, 0, length_with_null);
}

bool load_configuration(struct configuration*, const char*);
void sighandler(evutil_socket_t signum, short events, void *user_data)
{
    if (events) { events = 0; }
    struct sighandler_payload *s = user_data;
    struct event_base *eb = s->event_base;

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
        load_configuration(s->config, APPLICATION_NAME);
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

struct parsed_arguments *parse_command_line(enum binary_type which_bin, int argc, char *argv[])
{
    struct parsed_arguments *args = malloc(sizeof(struct parsed_arguments));
    args->log_level = warning | error;
    args->has_pid = false;
    args->has_help = false;
    args->service = unknown;

    char *name = argv[0];

    char *argument = NULL;
    if (argc > 0) { argument = argv[1]; }

    size_t name_len = strlen(name);
    args->name = get_zero_string(name_len);
    strncpy(args->name, name, name_len);
    size_t new_len = string_trim(&args->name, strlen(name), "./");

    for (int i = 0 ; i < argc; i++) {
        argument = argv[i];
        if (strcmp(argument, ARG_HELP) == 0) {
            args->has_help = true;
            break;
        }
        if (strncmp(argument, ARG_VERBOSE3, strlen(ARG_VERBOSE3)) == 0) {
            args->log_level = debug | info | warning | error;
#ifdef DEBUG
            args->log_level = args->log_level | tracing;
#else
            _warn("main::debug: extra verbose output is disabled");
#endif
            continue;
        }
        if (strncmp(argument, ARG_VERBOSE2, strlen(ARG_VERBOSE2)) == 0) {
            args->log_level = debug | info | warning | error;
            continue;
        }
        if (strncmp(argument, ARG_VERBOSE1, strlen(ARG_VERBOSE1)) == 0) {
            args->log_level = info | warning | error;
            continue;
        }
        if (strncmp(argument, ARG_QUIET, strlen(ARG_QUIET)) == 0) {
            args->log_level = error;
            continue;
        }
        switch (which_bin) {
            case (daemon_bin):
                if (strncmp(argument, ARG_PID, strlen(ARG_PID)) == 0) {
                    args->has_pid = true;
                    if (i + 1 <= argc && argv[i+1][0] != '-') {
                        args->pid_path = argv[i+1];
                        i += 1;
                    }
                }
                break;
            case (signon_bin):
                if (strncmp(argument, ARG_LASTFM, strlen(ARG_LASTFM)) == 0) {
                    args->service = lastfm;
                }
                if (strncmp(argument, ARG_LIBREFM, strlen(ARG_LIBREFM)) == 0) {
                    args->service = librefm;
                }
                break;
        }
    }
    extern enum log_levels _log_level;
    _log_level = args->log_level;
    _info("main::application_name[%u]: %s", new_len, args->name);

    return args;
}

const char* get_version(void)
{
#ifndef VERSION_HASH
#define VERSION_HASH "(unknown)"
#endif
    return VERSION_HASH;
}

#endif // MPRIS_SCROBBLER_UTILS_H
