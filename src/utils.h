/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_UTILS_H
#define MPRIS_SCROBBLER_UTILS_H

#define _GNU_SOURCE
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

#define array_count(a) (sizeof(a)/sizeof 0[a])
#define imax(a, b) ((a > b) ? b : a)

char *get_zero_string(size_t length)
{
    size_t length_with_null = (length + 1) * sizeof(char);
    char *result = calloc(1, length_with_null);
#if 0
    if (NULL == result) {
        _trace2("mem::could_not_allocate %lu bytes string", length_with_null);
    } else {
        _trace2("mem::allocated %lu bytes string", length_with_null);
    }
#endif

    return result;
}

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
    if (level_is(l, tracing2)) { return LOG_TRACING_LABEL; }
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
#define _trace2(...) _log(tracing2, __VA_ARGS__)

static int _log(enum log_levels level, const char *format, ...)
{
#ifndef DEBUG
    if (level_is(level, tracing)) { return 0; }
    if (level_is(level, tracing2)) { return 0; }
#endif

    extern enum log_levels _log_level;
    if (!level_is(_log_level, level)) { return 0; }

    va_list args;
    va_start(args, format);

    const char *label = get_log_level(level);
    size_t p_len = strlen(LOG_WARNING_LABEL) + 1;

    const char *suffix = "\n";
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
    //fprintf(stderr, "checking %p %p %d %s\n", string, *string, **string, *string);
    if (NULL == *string) { return 0; }
    if (NULL == remove) { remove = "\t\r\n "; }
    //fprintf(stderr, "checking %s vs. %s\n", *string, remove);

    size_t new_len = 0;
    size_t st_pos = 0;
    size_t end_pos = len;

    for (size_t i = 0; i < len; i++) {
        char byte = (*string)[i];
        //fprintf(stderr, "checking %c vs. %s\n", byte, remove);
        if (strchr(remove, byte)) {
            st_pos = i + 1;
        } else {
            break;
        }
    }

    if (st_pos < end_pos) {
        for (size_t i = end_pos; i > st_pos; i--) {
            char byte = (*string)[i-1];
            //fprintf(stderr, "checking %c vs. %s\n", byte, remove);
            if (strchr(remove, byte)) {
                end_pos = i;
            } else {
                break;
            }
        }
    }
    new_len = end_pos - st_pos;
    if (st_pos > 0 && end_pos > 0 && new_len > 0) {
        char *new_string = get_zero_string(new_len);
        strncpy(new_string, *string + st_pos, new_len);
        free(*string);
        *string = new_string;
    }
    //fprintf(stderr, "new name %p %p %d %s\n", string, *string, **string, *string);

    return new_len;
}

#if 0
enum api_type get_api_type(const char *label)
{
    if (strncmp(label, SERVICE_LABEL_LASTFM, strlen(SERVICE_LABEL_LASTFM))) { return lastfm; }
    if (strncmp(label, SERVICE_LABEL_LIBREFM, strlen(SERVICE_LABEL_LASTFM))) { return librefm; }
    if (strncmp(label, SERVICE_LABEL_LISTENBRAINZ, strlen(SERVICE_LABEL_LASTFM))) { return listenbrainz; }

    return unknown;
}
#endif

static const char *get_api_type_label(enum api_type end_point)
{
    switch (end_point) {
        case(lastfm):
            return "last.fm";
            break;
        case(librefm):
            return "libre.fm";
            break;
        case(listenbrainz):
            return "listenbrainz.org";
            break;
        case(unknown):
        default:
            return "unknown";
            break;
    }

    return NULL;
}

void zero_string(char **incoming, size_t length)
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
    }
    if (signum == SIGINT || signum == SIGTERM) {
        event_base_loopexit(eb, NULL);
    }
}

void free_arguments(struct parsed_arguments *args)
{
    if (NULL == args->url) { free(args->url); }
    if (NULL == args->name) { free(args->name); }
    if (NULL == args->pid_path) { free(args->pid_path); }
    free(args);
}

#define VERBOSE_TRACE2 "vvv"
#define VERBOSE_TRACE  "vv"
#define VERBOSE_DEBUG  "v"

struct parsed_arguments *parse_command_line(enum binary_type which_bin, int argc, char *argv[])
{
    struct parsed_arguments *args = malloc(sizeof(struct parsed_arguments));
    args->get_token = false;
    args->get_session = false;
    args->has_help = false;
    args->has_url = false;
    args->disable = false;
    args->enable = false;
    args->pid_path = NULL;
    args->url = NULL;
    args->service = unknown;
    args->log_level = warning | error;

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
                    args->service = lastfm;
                }
                if (strncmp(optarg, ARG_LIBREFM, strlen(ARG_LIBREFM)) == 0) {
                    args->service = librefm;
                }
                if (strncmp(optarg, ARG_LISTENBRAINZ, strlen(ARG_LISTENBRAINZ)) == 0) {
                    args->service = listenbrainz;
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
                args->log_level = error;
                break;
            case 'v':
                if (args->log_level == error) { break; }
                if (NULL == optarg) {
                    args->log_level = info | warning | error;
                    break;
                }
                if (strncmp(optarg, VERBOSE_TRACE, strlen(VERBOSE_TRACE)) == 0 || strtol(optarg, NULL, 10) >= 3) {
                    args->log_level = debug | info | warning | error;
#ifdef DEBUG
                    args->log_level |= tracing;
                    if (strncmp(optarg, VERBOSE_TRACE2, strlen(VERBOSE_TRACE2)) == 0 || strtol(optarg, NULL, 10) >= 4) {
                        args->log_level |= tracing2;
                    }
#else
                    _warn("main::debug: extra verbose output is disabled");
#endif
                    break;
                }
                if (strncmp(optarg, VERBOSE_DEBUG, strlen(VERBOSE_DEBUG)) == 0 || strtol(optarg, NULL, 10) == 2) {
                    args->log_level = debug | info | warning | error;
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

    return args;
}

#ifndef VERSION_HASH
#include "version.h"
#endif
const char *get_version(void)
{
#ifdef VERSION_HASH
    return VERSION_HASH;
#else
    return "(unknown)";
#endif
}

size_t string_array_resize(char ***s, size_t old_length, size_t new_length)
{
    _trace2("mem::resizing_string_array[%p::%zu::%zu]", s, old_length, new_length);
    if (old_length == new_length) { return old_length; }

    if (old_length < new_length) {
        char **temp  = realloc(*s, new_length * sizeof(char*));
        if (NULL == temp) {
            _error("mem::resizing_string_array_allocation_failed");
            return old_length;
        }
        *s = temp;
        _trace2("mem::allocated_new_size[%p::%zu]", s, new_length);

        for (size_t i = old_length; i < new_length; i++) {
            *s[i] = get_zero_string(MAX_PROPERTY_LENGTH);
            _trace2("\tmem::allocating::new_string[%zu:%p]", i, *s[i]);
        }
    } else {
        for (size_t i = new_length; i < old_length; i++) {
            free((*s)[i]);
            _trace2("\tmem::freeing::old_string[%zu:%p]", i, *s[i]);
        }
        *s  = realloc(*s, new_length * sizeof(char*));
    }
    return new_length;
}

void string_array_copy(char **d, size_t old_length, const char **s, size_t new_length)
{

    if (NULL == s) { return; }
    if (NULL == d) { return; }

    if (new_length != old_length) {
#if 0
        len = string_array_resize(d, old_length, new_length);
#endif
    }
    for (size_t i = 0; i < new_length; i++) {
        _trace2("\tmem::copying_string[%2zu::%p->%p]: %s", i, s[i], d[i], d[i]);
        memcpy(d[i], s[i], strlen(s[i]));
    }
}

void string_array_zero (char **arr, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        if (NULL != arr[i]) {
            size_t str_len = strlen(arr[i]);
            if (str_len > 0) {
                memset(arr[i], 0, str_len);
            }
        }
    }
}

void string_array_free(char **arr, size_t length)
{
    if (NULL == arr) { return; }

    _trace2("mem::freeing_string_array[%2zu::%p]", length, arr);
    for (size_t i = 0; i < length; i++) {
        if (NULL != arr[i]) {
            _trace2("\tmem::freeing::string[%2zu:%p]", i, (void*)arr[i]);
            free(arr[i]);
            arr[i] = NULL;
        }
    }
    free(arr);
}

char **string_array_new(size_t length, size_t str_len)
{
    if (length == 0) { return NULL; }

    char **str_arr = calloc(length, sizeof(char *));
    for (size_t i = 0; i < length; i++) {
        str_arr[i] = get_zero_string(str_len);
        _trace2("\tmem::allocated::new_string[%zu:%p]", i, str_arr[i]);
    }
    return str_arr;
}

#endif // MPRIS_SCROBBLER_UTILS_H
