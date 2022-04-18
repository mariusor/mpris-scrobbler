/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_UTILS_H
#define MPRIS_SCROBBLER_UTILS_H

#include <assert.h>
#include <event.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum log_levels _log_level;

#define _neg(a, b) (a) &= ~(b);

#define _eq(a, b) (memcmp((void*)&(a), (void*)&(b), sizeof(a)) == 0)
#define _cpy(a, b) memcpy(&(a), &(b), sizeof(a))

#define _to_bool(a) (a ? "yes" : "no")

#define array_count(a) (sizeof(a)/sizeof 0[a])
#define max(a, b) (((a) >= (b)) ? a : b)
#define min(a, b) (((a) <= (b)) ? a : b)
#define zero_string(incoming, length) memset(&incoming, 0, (length + 1) * sizeof(char))
#define get_zero_string(len) grrrs_new(len)
#define string_free(s) grrrs_free(s)

#define LOG_ERROR_LABEL "ERROR"
#define LOG_WARNING_LABEL "WARNING"
#define LOG_DEBUG_LABEL "DEBUG"
#define LOG_INFO_LABEL "INFO"
#define LOG_TRACING_LABEL "TRACING"
#define LOG_TRACING2_LABEL "TRACING"

static bool level_is(unsigned incoming, enum log_levels level)
{
    return ((incoming & level) == level);
}

static const char *get_log_level (enum log_levels l)
{
    if (level_is(l, log_tracing2)) { return LOG_TRACING_LABEL; }
    if (level_is(l, log_tracing)) { return LOG_TRACING_LABEL; }
    if (level_is(l, log_debug)) { return LOG_DEBUG_LABEL; }
    if (level_is(l, log_info)) { return LOG_INFO_LABEL; }
    if (level_is(l, log_warning)) { return LOG_WARNING_LABEL; }
    if (level_is(l, log_error)) { return LOG_ERROR_LABEL; }
    return LOG_TRACING_LABEL;
}

#define _log(level, format, ...) _logd(level, "", "", 0, format, __VA_ARGS__)
#define _error(...) _logd(log_error, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define _warn(...) _logd(log_warning, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define _info(...) _logd(log_info, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define _debug(...) _logd(log_debug, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define _trace(...) _logd(log_tracing, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define _trace2(...) _logd(log_tracing2, __FILE__, __func__, __LINE__, __VA_ARGS__)

void trim_path(const char *path, char *destination, int length)
{
    char *dirpath = grrrs_from_string(path);
    char *dir = dirname(dirpath);
    char *basedir = basename(dir);

    char *basepath = grrrs_from_string(path);
    char *base = basename(basepath);

    snprintf(destination, length, "%s/%s", basedir, base);
    grrrs_free(dirpath);
    grrrs_free(basepath);
}

#define GRAY_COLOUR "\033[38;5;240m"
#define RESET_COLOUR "\033[0m"

int _logd(enum log_levels level, const char *file, const char *function, const int line, const char *format, ...)
{
    extern enum log_levels _log_level;
#if !DEBUG
    if (level >= log_tracing) { return 0; }
    (void)file;
    (void)function;
    (void)line;
#endif
    if (!level_is(_log_level, level)) { return 0; }

    FILE *out = stdout;
    if (level < log_info) {
        out = stderr;
    }

    va_list args;
    va_start(args, format);

    const char *label = get_log_level(level);

    size_t f_len = strlen(format);
    char log_format[10240] = {0};
    snprintf(log_format, 10240-1, "%-7s ", label);

    strncat(log_format, format, f_len + 1);
    char suffix[1024] = {"\n"};
#if DEBUG
    if (level > log_debug && strlen(function) > 0 && strlen(file) > 0 && line > 0) {
        char path[256] = {0};
        trim_path((char*)file, path, 256);
        snprintf(suffix, 1024, GRAY_COLOUR " in %s() %s:%d\n" RESET_COLOUR, function, path, line);
    }
#endif
    strncat(log_format, suffix, 1024);

    int result = vfprintf(out, log_format, args);
    va_end(args);
    fflush(out);

    return result;
}

void array_log_with_label(char *output, const char arr[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH], int len)
{
    if (len <= 0) { return; }

    memset(output, 0, MAX_PROPERTY_LENGTH*MAX_PROPERTY_COUNT+9);

    char temp[MAX_PROPERTY_COUNT*MAX_PROPERTY_LENGTH+1] = {0};
    unsigned short cnt = 0;
    for (int i = 0; i < len; i++) {
        if (arr[i] == NULL || strlen(arr[i]) == 0) {
            break;
        }
        if (i > 0) {
            memcpy(temp + strlen(temp), ", ", 2);
        }
        memcpy(temp + strlen(temp), arr[i], strlen(arr[i]));
        cnt++;
    }
    if (cnt > 1) {
        snprintf(output, MAX_PROPERTY_LENGTH*MAX_PROPERTY_COUNT+8, "[%u]: %s", cnt, temp);
    } else {
        snprintf(output, MAX_PROPERTY_LENGTH*MAX_PROPERTY_COUNT+1, "%s", temp);
    }
}

const char *get_api_type_label(enum api_type end_point)
{
    switch (end_point) {
        case(api_lastfm):
            return "last.fm";
            break;
        case(api_librefm):
            return "libre.fm";
            break;
        case(api_listenbrainz):
            return "listenbrainz.org";
            break;
        case(api_unknown):
        default:
            return "unknown";
            break;
    }

    return NULL;
}

void resend_now_playing (struct state *);
bool load_configuration(struct configuration*, const char*);
void sighandler(evutil_socket_t signum, short events, void *user_data)
{
    if (events) { events = 0; }
    struct state *s = user_data;
    struct event_base *eb = s->events.base;

    const char *signal_name = "UNKNOWN";
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
        resend_now_playing(s);
    }
    if (signum == SIGINT || signum == SIGTERM) {
        event_base_loopexit(eb, NULL);
    }
}
void arguments_clean(struct parsed_arguments *args)
{
    if (NULL == args->url) { string_free(args->url); }
    if (NULL == args->name) { string_free(args->name); }
    if (NULL == args->pid_path) { string_free(args->pid_path); }
}

void free_arguments(struct parsed_arguments *args)
{
    arguments_clean(args);
    free(args);
}

#define VERBOSE_TRACE2 "vvv"
#define VERBOSE_TRACE  "vv"
#define VERBOSE_DEBUG  "v"

void parse_command_line(struct parsed_arguments *args, enum binary_type which_bin, int argc, char *argv[])
{
    args->get_token = false;
    args->get_session = false;
    args->has_help = false;
    args->has_url = false;
    args->disable = false;
    args->enable = false;
    args->url = NULL;
    args->service = api_unknown;
    args->log_level = log_warning | log_error;

    args->name = basename(argv[0]);

    int option_index = 0;

    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"quiet", no_argument, NULL, 'q'},
        {"verbose", optional_argument, NULL, 'v'},
        {"url", required_argument, NULL, 'u'},
    };
    opterr = 0;
    while (true) {
        int char_arg = getopt_long(argc, argv, "-hqu:v::", long_options, &option_index);
        if (char_arg == -1) { break; }
        switch (char_arg) {
            case 1:
                if (which_bin == daemon_bin) { break; }
                if (strncmp(optarg, ARG_LASTFM, strlen(ARG_LASTFM)) == 0) {
                    args->service = api_lastfm;
                }
                if (strncmp(optarg, ARG_LIBREFM, strlen(ARG_LIBREFM)) == 0) {
                    args->service = api_librefm;
                }
                if (strncmp(optarg, ARG_LISTENBRAINZ, strlen(ARG_LISTENBRAINZ)) == 0) {
                    args->service = api_listenbrainz;
                }
                if (strncmp(optarg, ARG_COMMAND_TOKEN, strlen(ARG_COMMAND_TOKEN)) == 0) {
                    args->get_token = true;
                }
                if (strncmp(optarg, ARG_COMMAND_SESSION, strlen(ARG_COMMAND_SESSION)) == 0) {
                    args->get_session = true;
                }
                if (strncmp(optarg, ARG_COMMAND_DISABLE, strlen(ARG_COMMAND_DISABLE)) == 0) {
                    args->disable = true;
                }
                if (strncmp(optarg, ARG_COMMAND_ENABLE, strlen(ARG_COMMAND_ENABLE)) == 0) {
                    args->enable = true;
                }
                break;
            case 'q':
                args->log_level = log_error;
                break;
            case 'v':
                if (args->log_level == log_error) { break; }
                if (NULL == optarg) {
                    args->log_level = log_info | log_warning | log_error;
                    break;
                }
                if (strncmp(optarg, VERBOSE_TRACE, strlen(VERBOSE_TRACE)) == 0 || strtol(optarg, NULL, 10) >= 3) {
                    args->log_level = log_debug | log_info | log_warning | log_error;
#ifdef DEBUG
                    args->log_level |= log_tracing;
                    if (strncmp(optarg, VERBOSE_TRACE2, strlen(VERBOSE_TRACE2)) == 0 || strtol(optarg, NULL, 10) >= 4) {
                        args->log_level |= log_tracing2;
                    }
#else
                    _warn("main::debug: extra verbose output is disabled");
#endif
                    break;
                }
                if (strncmp(optarg, VERBOSE_DEBUG, strlen(VERBOSE_DEBUG)) == 0 || strtol(optarg, NULL, 10) == 2) {
                    args->log_level = log_debug | log_info | log_warning | log_error;
                    break;
                }
                break;
            case 'u':
                args->has_url = true;
                args->url = optarg;
                break;
            case 'h':
                args->has_help = true;
            case '?':
            default:
                break;
        }
    }
    extern enum log_levels _log_level;
    _log_level = args->log_level;
}

const char *get_version(void)
{
#ifndef VERSION_HASH
#include "version.h"
#endif
    return VERSION_HASH;
}

#endif // MPRIS_SCROBBLER_UTILS_H
