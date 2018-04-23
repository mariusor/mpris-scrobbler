/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_INI_BASE_H
#define MPRIS_SCROBBLER_INI_BASE_H

#include <errno.h>
#include <string.h>

typedef struct ini_value {
    struct ini_group *parent;
    char *key;
    char *value;
} ini_value;

typedef struct ini_group {
    char *name;
    ini_value **values;
} ini_group;

typedef struct ini_config {
    ini_group **groups;
} ini_config;

void ini_value_free(ini_value *value)
{
    if (NULL == value) { return; }
    if (NULL != value->key) { free(value->key); }
    if (NULL != value->value) { free(value->value); }
    free(value);
}

void ini_group_free(ini_group *group)
{
    if (NULL == group) { return; }

    int count = sb_count(group->values);
    for (int i = 0; i < count; i++) {
        ini_value_free(group->values[i]);
        (void)sb_add(group->values, (-1));
    }
    assert(sb_count(group->values) == 0);
    sb_free(group->values);
    if (NULL != group->name) { free(group->name); }
    free(group);
}

void ini_config_free(ini_config *conf)
{
    if (NULL == conf) { return; }

    int count = sb_count(conf->groups);
    for (int i = 0; i < count; i++) {
        ini_group_free(conf->groups[i]);
        (void)sb_add(conf->groups, (-1));
    }
    assert(sb_count(conf->groups) == 0);
    sb_free(conf->groups);
    free(conf);
}

ini_value *ini_value_new(char *key, char *value)
{
    ini_value *val = calloc(1, sizeof(ini_value));

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

ini_group *ini_group_new(char *group_name)
{
    ini_group *group = calloc(1, sizeof(ini_group));
    group->values = NULL;

    group->name = calloc(1, MAX_PROPERTY_LENGTH);
    if (NULL != group_name) {
        strncpy(group->name, group_name, strlen(group_name));
    }

    return group;
}

ini_config *ini_config_new(void)
{
    ini_config *conf = calloc(1, sizeof(ini_config));
    conf->groups = NULL;

    return conf;
}

void ini_group_append_value (ini_group *group, ini_value *value)
{
    if (NULL == group) { return; }
    if (NULL == value) { return; }

    sb_push(group->values, value);
}

void ini_config_append_group (ini_config *conf, ini_group *group) {
    if (NULL == conf) { return; }
    if (NULL == group) { return; }

    sb_push(conf->groups, group);
}

void print_ini(struct ini_config *conf)
{
    if (NULL == conf) { return; }

    int group_count = sb_count(conf->groups);
    for (int i = 0; i < group_count; i++) {
        printf ("[%s]\n", conf->groups[i]->name);

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

    int group_count = sb_count(config->groups);
    for (int i = 0; i < group_count; i++) {
        fprintf (file, "[%s]\n", config->groups[i]->name);

        int value_count = sb_count(config->groups[i]->values);
        for (int j = 0; j < value_count; j++) {
            fprintf(file, "%s = %s\n", config->groups[i]->values[j]->key, config->groups[i]->values[j]->value);
        }
        fprintf(file, "\n");
    }

    return status;
}

#endif // MPRIS_SCROBBLER_INI_BASE_H
