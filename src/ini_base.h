/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_INI_BASE_H
#define MPRIS_SCROBBLER_INI_BASE_H

#include "stretchy_buffer.h"

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
    if (NULL != value->key) { free(value->key); }
    if (NULL != value->value) { free(value->value); }
    free(value);
}

static void ini_group_free(struct ini_group *group)
{
    if (NULL == group) { return; }

    if (NULL != group->name) { free(group->name); }
    if (NULL != group->values) {
        int count = sb_count(group->values);
        for (int i = 0; i < count; i++) {
            ini_value_free(group->values[i]);
            (void)sb_add(group->values, (-1));
        }
        assert(sb_count(group->values) == 0);
        sb_free(group->values);
    }
    free(group);
}

void ini_config_clean (struct ini_config *conf)
{
    if (NULL == conf->groups) { goto _free_sb; }

    int count = sb_count(conf->groups);
    for (int i = 0; i < count; i++) {
        ini_group_free(conf->groups[i]);
        (void)sb_add(conf->groups, (-1));
    }
    assert(sb_count(conf->groups) == 0);
_free_sb:
    sb_free(conf->groups);
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
        strncpy(val->key, key, strlen(key));
    }
    val->value = calloc(1, MAX_PROPERTY_LENGTH);
    if (NULL != value) {
        strncpy(val->value, value, strlen(value));
    }

    return val;
}

static struct ini_group *ini_group_new(char *group_name)
{
    struct ini_group *group = calloc(1, sizeof(struct ini_group));
    group->values = NULL;

    group->name = calloc(1, MAX_PROPERTY_LENGTH);
    if (NULL != group_name) {
        strncpy(group->name, group_name, strlen(group_name));
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

    sb_push(group->values, value);
}

static void ini_config_append_group (struct ini_config *conf, struct ini_group *group)
{
    if (NULL == conf) { return; }
    if (NULL == group) { return; }

    sb_push(conf->groups, group);
}

void print_ini(struct ini_config *conf)
{
    if (NULL == conf) { return; }
    if (NULL == conf->groups) { return; }

    int group_count = sb_count(conf->groups);
    for (int i = 0; i < group_count; i++) {
        printf ("[%s]\n", conf->groups[i]->name);

        if (NULL == conf->groups[i]->values) { return; }
        int value_count = sb_count(conf->groups[i]->values);
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
        int group_count = sb_count(config->groups);
        for (int i = 0; i < group_count; i++) {
            fprintf (file, "[%s]\n", config->groups[i]->name);

            if (NULL != config->groups[i]->values) {
                int value_count = sb_count(config->groups[i]->values);
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
