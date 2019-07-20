/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SSTRINGS_H
#define MPRIS_SCROBBLER_SSTRINGS_H

#ifdef DEBUG
#include <assert.h>
#else
#define assert(A)
#endif

#include <inttypes.h>

#ifndef grrrs_std_alloc
#include <stdlib.h>
#define grrrs_std_alloc malloc
#endif

#ifndef grrrs_std_realloc
#include <stdlib.h>
#define grrrs_std_realloc realloc
#endif

#ifndef grrrs_std_free
#include <stdlib.h>
#define grrrs_std_free free
#endif

#ifndef GRRRS_OOM
#define GRRRS_OOM
#endif

#ifndef GRRRS_ERR
#define GRRRS_ERR(...)
#endif

#define internal static
#define _VOID(A) (NULL == (A))
#define _OKP(A) (NULL != (A))
#define _GRRRS_NULL_TOP_PTR (-2 * sizeof(uint32_t))

#define _grrr_sizeof(C) (sizeof(struct grrr_string) + ((C+1) * sizeof(char)))

#define grrrs_from_string(A) (_VOID(A) ? \
    (char *)&_grrrs_new_empty(0)->data : \
    (char *)&_grrrs_new_from_cstring(A)->data)

#define grrrs_new(A) (char*)&(_grrrs_new_empty(A)->data)
#define grrrs_free(A) _grrrs_free(A)

// TODO(marius): investigate how this aligns
struct grrr_string {
    uint32_t len; /* currently used size of data[] */
    uint32_t cap; /* available size for data[] */
    char data[];
};

//typedef struct grrr_string grrrs;

internal struct grrr_string *_grrrs_ptr(char *s)
{
    return (struct grrr_string*)(s + _GRRRS_NULL_TOP_PTR);
}


internal void _grrrs_free(char *s)
{
    if (_VOID(s)) { return; }

    struct grrr_string *gs = _grrrs_ptr(s);

    if ((void*)s != (void*)&gs->data) {
        // TODO(marius): Add some more checks to see if memory layout matches.
        return;
    }

    if (_VOID(gs->data)) {
        grrrs_std_free(gs->data);
    }
    grrrs_std_free(gs);
}

internal struct grrr_string *_grrrs_new_empty(size_t cap)
{
    struct grrr_string *result = grrrs_std_alloc(_grrr_sizeof(cap));
    if (_VOID(result)) {
        GRRRS_OOM;
        return (void*)_GRRRS_NULL_TOP_PTR;
    }

    result->len = 0;
    result->cap = cap;
    for (size_t i = 0; i <= cap; i++) {
        result->data[i] = '\0';
    }

    return result;
}

internal uint32_t __strlen(const char *s)
{
    if (_VOID(s)) { return 0; }

    uint32_t result = 0;

    while (*s++ != '\0') { result++; }

    return result;
}

internal struct grrr_string *_grrrs_new_from_cstring(const char* s)
{
    int len = __strlen(s);
    struct grrr_string *result = grrrs_std_alloc(_grrr_sizeof(len));
    if (_VOID(result)) {
        GRRRS_OOM;
        return (void*)_GRRRS_NULL_TOP_PTR;
    }

    result->len = len;
    result->cap = len;

    for (int i = 0; i < len; i++) {
        result->data[i] = s[i];
    }
    result->data[len] = '\0';

    return result;
}

uint32_t grrrs_cap(const char* s)
{
#ifdef DEBUG
    assert(_OKP(s));
#endif
    struct grrr_string *gs = _grrrs_ptr((char*)s);

#ifdef DEBUG
    assert(_OKP(gs));
    assert(__strlen(s) <= gs->cap);
    assert(__strlen(s) == gs->len);
#endif
    return gs->cap;
}

uint32_t grrrs_len(const char* s)
{
#ifdef DEBUG
    assert(_OKP(s));
#endif
    struct grrr_string *gs = _grrrs_ptr((char*)s);

#ifdef DEBUG
    assert(_OKP(gs));
#endif

#ifdef DEBUG
    assert(gs->data == s);
    assert(__strlen(gs->data) == gs->len);
#endif
    return gs->len;
}

int grrrs_cmp(const char *s1, const char *s2)
{
#ifdef DEBUG
    assert(_OKP(s1));
    assert(_OKP(s2));
#endif
    struct grrr_string *gs1 = _grrrs_ptr((char*)s1);
    struct grrr_string *gs2 = _grrrs_ptr((char*)s2);

#ifdef DEBUG
    assert(_OKP(gs1));
    assert(_OKP(gs2));
#endif

    if (gs1->len < gs2->len) {
        return -1;
    }
    if (gs1->len > gs2->len) {
        return 1;
    }

    for (uint32_t i = 0; i < gs1->len; i++) {
        if (gs1->data[i] == '\0') {
            GRRRS_ERR("NULL value in string data before length[%" PRIu32 ":%" PRIu32 "]", i, gs1->len);
        }
        if (gs2->data[i] == '\0') {
            GRRRS_ERR("NULL value in string data before length[%" PRIu32 ":%" PRIu32 "]", i, gs2->len);
        }
        if (gs1->data[i] < gs2->data[i]) {
            return -1;
        }
        if (gs1->data[i] > gs2->data[i]) {
            return 1;
        }
    }

    return 0;
}

internal struct grrr_string *__grrrs_resize(struct grrr_string *gs, uint32_t new_cap)
{
#ifdef DEBUG
    assert(_OKP(gs));
#endif
    if (new_cap < gs->len) {
        GRRRS_ERR("new cap should be larger than existing length %" PRIu32 " \n", gs->len);
    }
    // TODO(marius): cover the case where new_cap is smaller than gs->len
    // and maybe when it's smaller than gs->cap
    gs = grrrs_std_realloc(gs, _grrr_sizeof(new_cap));
    if (_VOID(gs)) {
        GRRRS_OOM ;
        return (void*)_GRRRS_NULL_TOP_PTR;
    }
    if ((uint32_t)new_cap < gs->cap) {
        // ensure existing string is null terminated
        gs->data[new_cap] = '\0';
    }

    for (unsigned i = gs->cap; i <= new_cap; i++) {
        // ensure that the new capacity is zeroed
        gs->data[i] = '\0';
    }
    gs->cap = new_cap;

    return gs;
}

void *_grrrs_resize(void *s, uint32_t new_cap)
{
#ifdef DEBUG
    assert(_OKP(s));
#endif
    struct grrr_string *gs = _grrrs_ptr((char*)s);
    return __grrrs_resize(gs, new_cap)->data;
}

void *_grrrs_trim_left(char *s, const char *c)
{
    char *result = s;
    char *to_trim = NULL;
    if (_VOID(s)) { return result; }

    struct grrr_string *gs = _grrrs_ptr(s);
    if (_VOID(gs)) { return result; }

    if (_VOID(c)) {
        to_trim = _grrrs_new_from_cstring(" \t\r\n")->data;
    } else {
        to_trim = _grrrs_new_from_cstring(c)->data;
    }
    uint32_t len_to_trim = grrrs_len(to_trim);

    int trim_end = -1;
    uint32_t new_len = gs->len;
    for (uint32_t i = 0; i < gs->len; i++) {
        for (uint32_t j = 0; j < len_to_trim; j++) {
            char t = to_trim[j];
            if (gs->data[i] == '\0') {
                break;
            }
            if (gs->data[i] == t) {
                new_len--;
                break;
            }
            if (j == len_to_trim - 1) {
                trim_end = i;
            }
        }
        if (trim_end >= 0) {
            break;
        }
    }
    if (new_len == gs->len) {
        goto _to_trim_free;
    }
    char *temp = grrrs_std_alloc((new_len+1)*sizeof(char));
    for (uint32_t k = 0; k < new_len; k++) {
        temp[k] = gs->data[trim_end + k];
    }
    for (uint32_t k = 0; k < new_len; k++) {
        gs->data[k] = temp[k];
    }
    for (uint32_t k = new_len; k < gs->len; k++) {
        gs->data[k] = '\0';
    }
    gs->len = (uint32_t)new_len;
    grrrs_std_free(temp);

_to_trim_free:
    _grrrs_free(to_trim);

    return result;
}

void *_grrrs_trim_right(char *s, const char *c)
{
    char *result = s;
    char *to_trim = NULL;
    if (_VOID(s)) { return result; }

    struct grrr_string *gs = _grrrs_ptr(s);
    if (_VOID(gs)) { return result; }

    if (_VOID(c)) {
        to_trim = _grrrs_new_from_cstring("\r \t\n")->data;
    } else {
        to_trim = _grrrs_new_from_cstring(c)->data;
    }
    uint32_t len_to_trim = grrrs_len(to_trim);

    //assert(gs->len, len(gs->data);

    int8_t stop = 0;
    uint32_t new_len = gs->len;
    for (int32_t i = gs->len - 1; i >= 0; i--) {
        for (uint32_t j = 0; j < len_to_trim; j++) {
            char t = to_trim[j];
            if (gs->data[i] == '\0') {
                break;
            }
            if (gs->data[i] == t) {
                gs->data[i] = '\0';
                new_len--;
                break;
            }
            if (j == len_to_trim - 1) {
                stop = 1;
            }
        }
        if (stop) {
            break;
        }
    }
    if (new_len == gs->len) {
        goto _to_trim_free;
    }
    gs->len = (uint32_t)new_len;

_to_trim_free:
    _grrrs_free(to_trim);

    return result;
}

#define grrrs_trim(A, B) _grrrs_trim_right(_grrrs_trim_left((A), (B)), (B))

#endif // MPRIS_SCROBBLER_SSTRINGS_H
