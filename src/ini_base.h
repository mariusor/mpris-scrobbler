/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_INI_BASE_H
#define MPRIS_SCROBBLER_INI_BASE_H

#include <errno.h>
#include <string.h>

#define MAX_LENGTH 100
#define MAX_ENTRIES 100

typedef struct ini_value {
    struct ini_group *parent;
    char *key;
    char *value;
} ini_value;

typedef struct ini_group {
    char *name;
    ini_value *values[MAX_ENTRIES];
    size_t values_count;
} ini_group;

typedef struct ini_config {
    ini_group *groups[MAX_ENTRIES];
    size_t groups_count;
} ini_config;

void ini_value_free(ini_value *value)
{
    if (NULL == value) { return; }
    if (NULL != value->key) { free(value->key); }
    if (NULL != value->value) {free(value->value); }
    free(value);
}

void ini_group_free(ini_group* group)
{
    if (NULL == group) { return; }

    size_t count = group->values_count;
    for (size_t i = 0; i < count; i++) {
        ini_value_free(group->values[i]);
        group->values_count--;
    }
    if (NULL != group->name) { free(group->name); }
    free(group);
}

void ini_config_free(ini_config *conf)
{
    if (NULL == conf) { return; }

    size_t count = conf->groups_count;
    for (size_t i = 0; i < count; i++) {
        ini_group_free(conf->groups[i]);
        conf->groups_count--;
    }
    free(conf);
}

ini_value *ini_value_new(char *key, char *value)
{
    ini_value *val = calloc(1, sizeof(ini_value));

    val->key = calloc(1, MAX_LENGTH);
    if (NULL != key) {
        strncpy(val->key, key, strlen(key));
    }
    val->value = calloc(1, MAX_LENGTH);
    if (NULL != value) {
        strncpy(val->value, value, strlen(value));
    }

    return val;
}

ini_group *ini_group_new(char *group_name)
{
    ini_group *group = calloc(1, sizeof(ini_group));
    group->values_count = 0;

    group->name = calloc(1, MAX_LENGTH);
    if (NULL != group_name) {
        strncpy(group->name, group_name, strlen(group_name));
    }

    return group;
}

ini_config *ini_config_new(void)
{
    ini_config *conf = calloc(1, sizeof(ini_config));
    conf->groups_count = 0;

    return conf;
}

void ini_group_append_value (ini_group *group, ini_value *value)
{
    if (NULL == group) { return; }
    if (NULL == value) { return; }

    group->values[group->values_count++] = value;
}

void ini_config_append_group (ini_config *conf, ini_group *group) {
    if (NULL == conf) { return; }
    if (NULL == group) { return; }

    conf->groups[conf->groups_count++] = group;
}

void print_ini(struct ini_config *conf)
{
    if (NULL == conf) { return; }

    for (size_t i = 0; i < conf->groups_count; i++) {
        printf ("[%s]\n", conf->groups[i]->name);

        for (size_t j = 0; j < conf->groups[i]->values_count; j++) {
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

    for (size_t i = 0; i < config->groups_count; i++) {
        fprintf (file, "[%s]\n", config->groups[i]->name);

        for (size_t j = 0; j < config->groups[i]->values_count; j++) {
            fprintf(file, "%s = %s\n", config->groups[i]->values[j]->key, config->groups[i]->values[j]->value);
        }
        fprintf(file, "\n");
    }

    return status;
}

#endif // MPRIS_SCROBBLER_INI_BASE_H
