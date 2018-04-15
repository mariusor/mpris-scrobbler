/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_STRINGS_H
#define MPRIS_SCROBBLER_STRINGS_H

#ifndef LOG_ERROR_LABEL
#define _error(...)
#define _warn(...)
#define _info(...)
#define _debug(...)
#define _trace(...)
#define _trace2(...)
#endif

#ifndef MAX_PROPERTY_LENGTH
#define MAX_PROPERTY_LENGTH 1024
#endif

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
    char **str_arr = calloc(length, sizeof(char *));
    for (size_t i = 0; i < length; i++) {
        str_arr[i] = get_zero_string(str_len);
        _trace2("\tmem::allocated::new_string[%zu:%p]", i, str_arr[i]);
    }
    return str_arr;
}

size_t string_array_resize(char ***s, size_t old_length, size_t new_length)
{
    if (NULL == s) { return 0; }

    _trace2("mem::resizing_string_array[%p::%zu::%zu]", s, old_length, new_length);

    char **st = *s;
    char **temp = NULL;

    if (new_length == 0) {
        string_array_free(st, old_length);
        return new_length;
    }

    if (old_length == new_length) { return old_length; }
    for (size_t i = 0; i < old_length; i++) {
        _trace2("mem::freeing_old_string[%zu::%p]", st[i], i);
        free(st[i]);
    }

    temp  = realloc(st, new_length * sizeof(char*));
    if (NULL == temp) {
        _error("mem::resizing_string_array_allocation_failed");
        return old_length;
    }

    for (size_t i = 0; i < new_length; i++) {
        temp[i] = get_zero_string(MAX_PROPERTY_LENGTH);
        _trace2("mem::allocating_new_string[%zu::%p]", temp[i], i);
    }
    st = temp;
    *s = st;

    return new_length;
}

void string_array_copy(char **d, size_t old_length, const char **s, size_t new_length)
{
    if (new_length == 0) { return; }

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

#endif // MPRIS_SCROBBLER_STRINGS_H
