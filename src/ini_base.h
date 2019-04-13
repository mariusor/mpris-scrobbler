/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_INI_BASE_H
#define MPRIS_SCROBBLER_INI_BASE_H


#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ini_value {
    struct ini_group *parent;
    char *key;
    char *value;
};

struct ini_group {
    char *name;
    struct ini_value **values;
};

struct ini_config {
    struct ini_group **groups;
};

static void ini_value_free(struct ini_value *value)
{
    if (NULL == value) { return; }
    if (NULL != value->key) { string_free(value->key); }
    if (NULL != value->value) { string_free(value->value); }
    free(value);
}

static void ini_group_free(struct ini_group *group)
{
    if (NULL == group) { return; }

    if (NULL != group->name) { string_free(group->name); }
    if (NULL != group->values) {
        int count = arrlen(group->values);
        for (int i = count - 1; i >= 0; i--) {
            ini_value_free(group->values[i]);
            (void)arrpop(group->values);
            group->values[i] = NULL;
        }
        assert(arrlen(group->values) == 0);
        arrfree(group->values);
        group->values = NULL;
    }
    free(group);
}

void ini_config_clean (struct ini_config *conf)
{
    if (NULL == conf->groups) { goto _free_sb; }

    int count = arrlen(conf->groups);
        for (int i = count - 1; i >= 0; i--) {
        ini_group_free(conf->groups[i]);
        (void)arrpop(conf->groups);
        conf->groups[i] = NULL;
    }
    assert(arrlen(conf->groups) == 0);
_free_sb:
    arrfree(conf->groups);
    conf->groups = NULL;
}

void ini_config_free(struct ini_config *conf)
{
    if (NULL == conf) { return; }

    ini_config_clean(conf);

    free(conf);
}

static struct ini_value *ini_value_new(char *key, char *value)
{
    struct ini_value *val = calloc(1, sizeof(struct ini_value));

    val->key = calloc(1, MAX_PROPERTY_LENGTH);
    if (NULL != key) {
        strncpy(val->key, key, MAX_PROPERTY_LENGTH);
    }
    val->value = calloc(1, MAX_PROPERTY_LENGTH);
    if (NULL != value) {
        strncpy(val->value, value, MAX_PROPERTY_LENGTH);
    }

    return val;
}

static struct ini_group *ini_group_new(char *group_name)
{
    struct ini_group *group = calloc(1, sizeof(struct ini_group));
    group->values = NULL;

    group->name = calloc(1, MAX_PROPERTY_LENGTH);
    if (NULL != group_name) {
        strncpy(group->name, group_name, MAX_PROPERTY_LENGTH);
    }

    return group;
}

struct ini_config *ini_config_new(void)
{
    struct ini_config *conf = calloc(1, sizeof(struct ini_config));
    conf->groups = NULL;

    return conf;
}

static void ini_group_append_value (struct ini_group *group, struct ini_value *value)
{
    if (NULL == group) { return; }
    if (NULL == value) { return; }

    arrput(group->values, value);
}

static void ini_config_append_group (struct ini_config *conf, struct ini_group *group)
{
    if (NULL == conf) { return; }
    if (NULL == group) { return; }

    arrput(conf->groups, group);
}

void print_ini(struct ini_config *conf)
{
    if (NULL == conf) { return; }
    if (NULL == conf->groups) { return; }

    int group_count = arrlen(conf->groups);
    for (int i = 0; i < group_count; i++) {
        printf ("[%s]\n", conf->groups[i]->name);

        if (NULL == conf->groups[i]->values) { return; }
        int value_count = arrlen(conf->groups[i]->values);
        for (int j = 0; j < value_count; j++) {
            printf("  %s = %s\n", conf->groups[i]->values[j]->key, conf->groups[i]->values[j]->value);
        }
    }
}

int write_ini_file(struct ini_config *config, FILE *file)
{
    if (NULL == file) { return ENOENT; }

    int status = EINVAL;
    if (NULL == config) { return status; }

    status = 0;

    if (NULL != config->groups) {
        int group_count = arrlen(config->groups);
        for (int i = 0; i < group_count; i++) {
            fprintf (file, "[%s]\n", config->groups[i]->name);

            if (NULL != config->groups[i]->values) {
                int value_count = arrlen(config->groups[i]->values);
                for (int j = 0; j < value_count; j++) {
                    fprintf(file, "%s = %s\n", config->groups[i]->values[j]->key, config->groups[i]->values[j]->value);
                }
            }
            fprintf(file, "\n");
        }
    }

    return status;
}

#endif // MPRIS_SCROBBLER_INI_BASE_H
