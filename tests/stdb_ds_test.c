#include <snow/snow.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

describe(stretchy_buffers) {
    it("Pushing string to stretchy buffer") {
        char** arr = NULL;

        asserteq(arr, NULL);
        asserteq(arrlen(arr), 0);

        char* first = calloc(12, sizeof(char));

        arrput(arr, first);

        assertneq(arr, NULL);

        asserteq(arrlen(arr), 1);

        for (unsigned int i = 0; i < arrlen(arr); i++) {
            asserteq(arr[i], first);
        }
        asserteq(arrpop(arr), first);

        arrfree(arr);
    };

    it ("Pushing multiple strings to stretchy buffer") {
        char** arr = NULL;

        asserteq(arr, NULL);
        asserteq(arrlen(arr), 0);

        for(int i = 0; i < 10; i++) {
            char *new_el = calloc(12, sizeof(char));
            arrput(arr, new_el);
            assertneq(new_el, NULL);
        }
        asserteq(arrlen(arr), 10);

        arrfree(arr);
    };

    it ("Popping items from a stretchy buffer") {
        char** arr = NULL;

        asserteq(arr, NULL);
        asserteq(arrlen(arr), 0);

        for(int i = 0; i < 10; i++) {
            char *new_el = "aaaaa";
            arrput(arr, new_el);
            assertneq(new_el, NULL);
        }
        // checking array reached desired length
        asserteq(arrlen(arr), 10);

        // checking shrinking by one element
        char *popped = arrpop(arr);
        asserteq(arrlen(arr), 9);
        assertneq(popped, NULL);

        // checking shrinking by one element
        popped = arrpop(arr);
        asserteq(arrlen(arr), 8);
        assertneq(popped, NULL);

        // checking shrinking by one element
        popped = arrpop(arr);
        asserteq(arrlen(arr), 7);
        assertneq(popped, NULL);

        // checking shrinking to zero elements
        arrdeln(arr, 0, 7);
        asserteq(arrlen(arr), 0);
        assertneq(arr, NULL);

        arrfree(arr);
    };

};

snow_main();
