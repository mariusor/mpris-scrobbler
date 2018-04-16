#include "./lib/snow/snow/snow.h"

#define get_zero_string(A) calloc(1, (A + 1) * sizeof(char))

#include "sstrings.h"

#define setup(name, size) \
    char** name = string_array_new(size, MAX_PROPERTY_LENGTH); \
    defer(string_array_free(name, size))

#define array_length(n) sizeof(n)/sizeof(n[0])

#define test_string "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"

describe(vector, {
    it("inits zero string array correctly", {
        char** arr = string_array_new(0, MAX_PROPERTY_LENGTH);

        assertneq(arr, NULL);

        string_array_free(arr, 0);
    });

    it("inits non-zero string array correctly", {
        char** arr = string_array_new(13, MAX_PROPERTY_LENGTH);

        assertneq(arr, NULL);
        for (size_t i = 0; i < 13; i++) {
            assertneq(arr[i], NULL);
        }

        string_array_free(arr, 13);
    });

#if 1
    it("resizes array up from zero correctly", {
        setup(arr, 0);

        assertneq(arr, NULL);

        size_t newlen = 13;

        string_array_resize(&arr, 0, newlen);

        assertneq(arr, NULL);

        for (size_t i = 0; i < newlen; i++) {
            strncpy(arr[i], test_string, MAX_PROPERTY_LENGTH);
            asserteq(strncmp(arr[i], test_string, MAX_PROPERTY_LENGTH), 0);
            assertneq(arr[i], NULL);
        }
        string_array_free(arr, newlen);
    });
#endif

    it("resizes array up correctly", {
        setup(arr, 3);

        assertneq(arr, NULL);

        size_t newlen = 13;

        string_array_resize(&arr, 3, newlen);

        assertneq(arr, NULL);

#if 1
        for (size_t i = 0; i < newlen; i++) {
            strncpy(arr[i], test_string, MAX_PROPERTY_LENGTH);
            assertneq(arr[i], NULL);
            asserteq(strncmp(arr[i], test_string, MAX_PROPERTY_LENGTH), 0);
        }
#endif
        string_array_free(arr, newlen);
    });
});

snow_main();
