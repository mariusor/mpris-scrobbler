#include "./lib/snow/snow/snow.h"

#include "stretchy_buffer.h"

describe(stretchy_buffs, {
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
            sb_add(arr, 1);
        }
        asserteq(sb_count(arr), 10);

        sb_free(arr);
    });

});

snow_main();
