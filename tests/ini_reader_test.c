#include "./lib/snow/snow/snow.h"

#define MAX_PROPERTY_LENGTH        512

#include "../src/ini.h"

#define MAX_FILE_SIZE 1048576
#define array_len(A) (sizeof(A)/sizeof(A[0]))
#define min(A,B) ((A) > (B) ? B : A)

static int load_file(FILE *file, char buff[MAX_FILE_SIZE])
{
    long file_size = 0l;
    if (NULL == file) { goto _exit; }

    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    file_size = min(file_size, MAX_FILE_SIZE);

    if (file_size <= 0) { goto _exit; }
    rewind(file);

    fread(buff, file_size, 1, file);

_exit:
    return file_size;
}

struct element_test {
    const char *key;
    const char *value;
};

struct group_test {
    const char *name;
    const int element_count;
    const struct element_test elements[10];
};

struct test_pair {
    const char *path;
    const int group_count;
    const struct group_test groups[10];
};

const struct test_pair tests[2] = {
    {
        .path = "../mocks/simple.ini",
        .group_count = 1,
        .groups = {
            {
                .element_count = 1,
                .name = "test",
                .elements = {
                    {
                        .key = "key",
                        .value = "value",
                    },
                },
            },
        },
    },
    {
        .path = "../mocks/credentials.ini",
        .group_count = 3,
        .groups = {
            {
                .element_count = 4,
                .name = "one",
                .elements = {
                    {
                        .key = "enabled",
                        .value = "true",
                    },
                    {
                        .key = "username",
                        .value = "tester",
                    },
                    {
                        .key = "token",
                        .value = "f00b4r",
                    },
                    {
                        .key = "session",
                        .value = "kerfuffle",
                    },
                },
            },
            {
                .element_count = 4,
                .name = "two",
                .elements = {
                    {
                        .key = "enabled",
                        .value = "1",
                    },
                    {
                        .key = "username",
                        .value = "s!mb4",
                    },
                    {
                        .key = "token",
                        .value = "test!321@@321",
                    },
                    {
                        .key = "session",
                        .value = "rwar32test",
                    },
                },
            },
            {
                .element_count = 2,
                .name = "and-three",
                .elements = {
                    {
                        .key = "enabled",
                        .value = "false",
                    },
                    {
                        .key = "token",
                        .value = "f00d-ale-c0ffee-41f-6419418c768a"
                    },
                },
            },
        },
    },
};

describe(ini_reader) {
    FILE *file = NULL;

    after_each() {
        if (NULL != file) { fclose(file); }
    };

    for (size_t __test_key = 0; __test_key < array_len(tests); __test_key++) {
        const struct test_pair test = tests[__test_key];
        const char *path = test.path;
        const int group_count = test.group_count;

        it ("opens ini file") {
            file = fopen(path, "r");
            assertneq(file, NULL);
        };

        it ("reads ini file") {
            char buff[MAX_FILE_SIZE];
            memset(&buff, '\0', MAX_FILE_SIZE);

            file = fopen(path, "re");

            load_file(file, buff);

            assertneq(strlen(buff), 0);
        };

        it ("parse ini file") {
            char buff[MAX_FILE_SIZE];
            memset(&buff, '\0', MAX_FILE_SIZE);
            file = fopen(path, "re");
            long buff_size = load_file(file, buff);

            struct ini_config config = { .groups = NULL, };
            ini_parse(buff, buff_size, &config);

            assertneq(config.groups, NULL);
            assertneq(sb_count(config.groups), 0);
            asserteq(sb_count(config.groups), group_count);

            for (int i = 0; i < sb_count(config.groups); i++) {
                struct ini_group *group = config.groups[i];

                assertneq(group, NULL);
                assertneq(group->name, NULL);
                asserteq(group->name, test.groups[i].name);

                assertneq(group->values, NULL);
                assertneq(sb_count(group->values), 0);

                asserteq(sb_count(group->values), test.groups[i].element_count);

                for (int j = 0; j < sb_count(group->values); j++) {
                    struct ini_value *value = group->values[j];

                    assertneq(value, NULL);
                    assertneq(value->key, NULL);
                    assertneq(value->value, NULL);

                    asserteq(value->key, test.groups[i].elements[j].key);
                    asserteq(value->value, test.groups[i].elements[j].value);

                    asserteq(strncmp(value->key, test.groups[i].elements[j].key, 100), 0);
                    asserteq(strncmp(value->value, test.groups[i].elements[j].value, 100), 0);
                }
            }
            if (NULL != config.groups) { ini_config_clean(&config); }
        };
    }
};

snow_main();
