#include "./lib/snow/snow/snow.h"

#include "stretchy_buffer.h"

describe(stretchy_buffers, {
    it("Pushing string to stretchy buffer", {
        char** arr = NULL;

        asserteq(arr, NULL);
        asserteq(sb_count(arr), 0);

        char* first = "first";

        sb_push(arr, first);

        assertneq(arr, NULL);

        asserteq(sb_count(arr), 1);

        for (int i = 0; i < sb_count(arr); i++) {
            asserteq(arr[i], first);
        }
        asserteq(sb_last(arr), first);

        sb_free(arr);
    });

    it ("Pushing multiple strings to stretchy buffer", {
        char** arr = NULL;

        asserteq(arr, NULL);
        asserteq(sb_count(arr), 0);

        for(int i = 0; i < 10; i++) {
            char *new_el = "aaaaa";
            sb_push(arr, new_el);
            assertneq(new_el, NULL);
        }
        asserteq(sb_count(arr), 10);

        sb_free(arr);
    });

    it ("Popping items from a stretchy buffer", {
        char** arr = NULL;

        asserteq(arr, NULL);
        asserteq(sb_count(arr), 0);

        for(int i = 0; i < 10; i++) {
            char *new_el = "aaaaa";
            sb_push(arr, new_el);
            assertneq(new_el, NULL);
        }
        // checking array reached desired length
        asserteq(sb_count(arr), 10);

        // checking shrinking by one element
        char **new_arr = sb_add(arr, (-1));
        asserteq(sb_count(arr), 9);
        assertneq(new_arr, NULL);

        // checking shrinking by one element
        new_arr = sb_add(arr, (-1));
        asserteq(sb_count(arr), 8);
        assertneq(new_arr, NULL);

        // checking shrinking by one element
        new_arr = sb_add(arr, (-1));
        asserteq(sb_count(arr), 7);
        assertneq(new_arr, NULL);

        // checking shrinking to zero elements
        new_arr = sb_add(arr, (-7));
        asserteq(sb_count(arr), 0);
        assertneq(new_arr, NULL);

        sb_free(arr);
    });

});

snow_main();
