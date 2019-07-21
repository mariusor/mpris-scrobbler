/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <stdio.h>

#ifndef assert
#include <assert.h>
#endif
#define GRRRS_OOM assert(!"Failed to allocate memory")

#include "sstrings.h"

#include <snow/snow.h>

void assert_grrrs(char *t) {
    struct grrr_string *gs = _grrrs_ptr(t);
    asserteq_ptr((void*)&gs->data, (void*)t);
    asserteq_ptr((void*)&gs->cap, (void*)(t - sizeof(gs->cap)));
    asserteq_ptr((void*)&gs->len, (void*)(t - sizeof(gs->cap) - sizeof(gs->len)));
    asserteq_ptr((void*)gs, (void*)(t - sizeof(gs->cap) - sizeof(gs->len)));
    asserteq_int(gs->len, strlen(t));
    asserteq_int(gs->cap, strlen(t));
}

describe(basic) {
    subdesc(plumbing) {
        it ("Check __strlen") {
            const char *t = "ana are mere";

            asserteq_int(__strlen(t), strlen(t));
        }
        it ("Check __strlen for zero length") {
            const char *t = "";

            asserteq_int(__strlen(t), strlen(t));
        }
        it ("Check __strlen for NULL string") {
            const char *t = NULL;

            asserteq_int(__strlen(t), 0);
        }
    }
    subdesc(pointer plumbing) {
        it("Check pointer addresses") {
            char *t = grrrs_from_string(0);

            defer(_grrrs_free(t));

            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 0);
            asserteq_buf(t, "", 1);

            assert_grrrs(t);
        }
    }

    subdesc(initialization) {
        it("Zero string") {
            char *t = grrrs_from_string(0);

            defer(_grrrs_free(t));

            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 0);
            asserteq_buf(t, "", 1);
        }

        it("From static string") {
            char *t = grrrs_from_string("ana are mere\n");

            defer(_grrrs_free(t));

            asserteq_int(grrrs_len(t), 13);
            asserteq_int(grrrs_cap(t), 13);
            asserteq_buf(t, "ana are mere\n", 14);

            assert_grrrs(t);
        }

        it("From heap string") {
            char *h = calloc(1, sizeof(char)*10 + 1);
            for (unsigned i = 0; i < 10; i++) {
                h[i] = 't';
            }
            char *t = grrrs_from_string(h);

            asserteq_int(grrrs_len(t), 10);
            asserteq_int(grrrs_cap(t), 10);
            asserteq_buf(t, "tttttttttt", 10);

            assert_grrrs(t);

            _grrrs_free(t);
            free(h);
        }
    }

    subdesc(operations) {
        it("Grow string") {
            char *t = grrrs_from_string(0);
            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 0);

            t = _grrrs_resize(t, 128);
            asserteq_int(grrrs_cap(t), 128);
            asserteq_int(grrrs_len(t), 0);

            char null_block[128] = {0};

            asserteq_int(memcmp(t, &null_block, 128), 0);
            _grrrs_free(t);
        }

        it("Shrink string") {
            char *t = grrrs_from_string(0);

            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 0);

            t = _grrrs_resize(t, 128);
            asserteq_int(grrrs_cap(t), 128);
            asserteq_int(grrrs_len(t), 0);

            t = _grrrs_resize(t, 1);
            asserteq_int(grrrs_cap(t), 1);
            asserteq_int(grrrs_len(t), 0);

            char null_block[1] = {0};
            asserteq_int(memcmp(t, &null_block, 1), 0);

            _grrrs_free(t);
        }
    }
    subdesc(comparisons) {
        it("Empty strings") {
            char *s1 = grrrs_from_string("");
            char *s2 = grrrs_from_string(0);

            defer(
                _grrrs_free(s1),
                _grrrs_free(s2)
            );

            size_t l1 = grrrs_len(s1);
            size_t l2 = grrrs_len(s2);
            assert(l1 == l2);

            asserteq_int(grrrs_cmp(s1, s2), 0);
            asserteq_int(grrrs_cmp(s2, s1), 0);
        }
        it("Different lengths") {
            char *s1 = grrrs_from_string("ana are mere");
            char *s2 = grrrs_from_string("ana are mere?");

            defer(
                _grrrs_free(s1),
                _grrrs_free(s2)
            );

            size_t l1 = grrrs_len(s1);
            size_t l2 = grrrs_len(s2);
            assert(l1 < l2);

            asserteq_int(grrrs_cmp(s1, s2), -1);
            asserteq_int(grrrs_cmp(s2, s1), 1);
        }
        it("Same lengths, different chars") {
            char *s1 = grrrs_from_string("ana are mere");
            char *s2 = grrrs_from_string("ana are merd");

            defer(
                _grrrs_free(s1),
                _grrrs_free(s2)
            );

            size_t l1 = grrrs_len(s1);
            size_t l2 = grrrs_len(s2);
            assert(l1 == l2);

            asserteq_int(grrrs_cmp(s1, s2), 1);
            asserteq_int(grrrs_cmp(s2, s1), -1);

        }
    }

    subdesc(trim_left) {
        it("no matches to trim") {
            char *t = grrrs_from_string("ana");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 3);
            asserteq_int(grrrs_cap(t), 3);

            _grrrs_trim_left(t, 0);
            asserteq_buf(t, "ana", 3);
            asserteq_int(grrrs_len(t), 3);
            asserteq_int(grrrs_cap(t), 3);
        }
        it("custom character 'a'") {
            char *t = grrrs_from_string("aana");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 4);
            asserteq_int(grrrs_cap(t), 4);

            _grrrs_trim_left(t, "a");
            asserteq_buf(t, "na", 2);
            asserteq_int(grrrs_len(t), 2);
            asserteq_int(grrrs_cap(t), 4);
        }
        it("trim to empty 'a'") {
            char *t = grrrs_from_string("aaaaaaaa");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 8);
            asserteq_int(grrrs_cap(t), 8);

            _grrrs_trim_left(t, "a");
            asserteq_buf(t, "", 1);
            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 8);
        }
        it("trim to empty 'ab'") {
            char *t = grrrs_from_string("abaabbba");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 8);
            asserteq_int(grrrs_cap(t), 8);

            _grrrs_trim_left(t, "ab");
            asserteq_buf(t, "", 1);
            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 8);
        }
        it("trim to empty default whitespace characters") {
            char *t = grrrs_from_string(" \t  \r  \n");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 8);
            asserteq_int(grrrs_cap(t), 8);

            _grrrs_trim_left(t, 0);
            asserteq_buf(t, "", 1);
            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 8);
        }
        it("trim default whitespace characters") {
            char *t = grrrs_from_string("       ana are mere\n");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 20);
            asserteq_int(grrrs_cap(t), 20);

            _grrrs_trim_left(t, 0);
            asserteq_buf(t, "ana are mere\n", 13);
            asserteq_int(grrrs_len(t), 13);
            asserteq_int(grrrs_cap(t), 20);
        }
    }
    subdesc(trim_right) {
        it("no matches to trim") {
            char *t = grrrs_from_string("ana");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 3);
            asserteq_int(grrrs_cap(t), 3);

            _grrrs_trim_right(t, 0);
            asserteq_buf(t, "ana", 3);
            asserteq_int(grrrs_len(t), 3);
            asserteq_int(grrrs_cap(t), 3);
        }
        it("custom character 'a'") {
            char *t = grrrs_from_string("aana");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 4);
            asserteq_int(grrrs_cap(t), 4);

            _grrrs_trim_right(t, "a");
            asserteq_buf(t, "aan", 3);
            asserteq_int(grrrs_len(t), 3);
            asserteq_int(grrrs_cap(t), 4);
        }
        it("trim to empty 'a'") {
            char *t = grrrs_from_string("aaaaaaaa");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 8);
            asserteq_int(grrrs_cap(t), 8);

            _grrrs_trim_right(t, "a");
            asserteq_buf(t, "", 1);
            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 8);
        }
        it("trim to empty 'ab'") {
            char *t = grrrs_from_string("abaabbba");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 8);
            asserteq_int(grrrs_cap(t), 8);

            _grrrs_trim_right(t, "ab");
            asserteq_buf(t, "", 1);
            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 8);
        }
        it("trim to empty default whitespace characters") {
            char *t = grrrs_from_string(" \t  \r  \n");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 8);
            asserteq_int(grrrs_cap(t), 8);

            _grrrs_trim_right(t, 0);
            asserteq_buf(t, "", 1);
            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 8);
        }
        it("trim default whitespace characters") {
            char *t = grrrs_from_string("       ana are mere\n");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 20);
            asserteq_int(grrrs_cap(t), 20);

            _grrrs_trim_right(t, 0);
            asserteq_buf(t, "       ana are mere", 19);
            asserteq_int(grrrs_len(t), 19);
            asserteq_int(grrrs_cap(t), 20);
        }
    }
    subdesc(trim_both) {
        it("no matches to trim") {
            char *t = grrrs_from_string("ana");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 3);
            asserteq_int(grrrs_cap(t), 3);

            grrrs_trim(t, 0);
            asserteq_buf(t, "ana", 3);
            asserteq_int(grrrs_len(t), 3);
            asserteq_int(grrrs_cap(t), 3);
        }
        it("custom character 'a'") {
            char *t = grrrs_from_string("aana");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 4);
            asserteq_int(grrrs_cap(t), 4);

            grrrs_trim(t, "a");
            asserteq_buf(t, "n", 1);
            asserteq_int(grrrs_len(t), 1);
            asserteq_int(grrrs_cap(t), 4);
        }
        it("trim to empty 'a'") {
            char *t = grrrs_from_string("aaaaaaaa");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 8);
            asserteq_int(grrrs_cap(t), 8);

            grrrs_trim(t, "a");
            asserteq_buf(t, "", 1);
            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 8);
        }
        it("trim to empty 'ab'") {
            char *t = grrrs_from_string("abaabbba");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 8);
            asserteq_int(grrrs_cap(t), 8);

            grrrs_trim(t, "ab");
            asserteq_buf(t, "", 1);
            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 8);
        }
        it("trim to empty default whitespace characters") {
            char *t = grrrs_from_string(" \t  \r  \n");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 8);
            asserteq_int(grrrs_cap(t), 8);

            grrrs_trim(t, 0);
            asserteq_buf(t, "", 1);
            asserteq_int(grrrs_len(t), 0);
            asserteq_int(grrrs_cap(t), 8);
        }
        it("trim default whitespace characters") {
            char *t = grrrs_from_string("       ana are mere\n");
            defer(_grrrs_free(t));

            assert_grrrs(t);
            asserteq_int(grrrs_len(t), 20);
            asserteq_int(grrrs_cap(t), 20);

            grrrs_trim(t, 0);
            asserteq_buf("ana are mere", t, 12);
            asserteq_int(grrrs_len(t), 12);
            asserteq_int(grrrs_cap(t), 20);
        }
        it("trim sequence that segfaults: \\020") {
            char *t = grrrs_from_string("\0 ");
            defer(_grrrs_free(t));

            assert_grrrs(t);

            grrrs_trim(t, NULL);
            asserteq_buf("", t, 1);
        }
    }
}

snow_main();
