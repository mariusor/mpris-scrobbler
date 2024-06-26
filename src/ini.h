/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_INI_H
#define MPRIS_SCROBBLER_INI_H

#include "ini_base.h"

#define DEFAULT_GROUP_NAME "base"
#define COMMENT_SEMICOLON  ';'
#define COMMENT_HASH       '#'
#define GROUP_OPEN         '['
#define GROUP_CLOSE        ']'
#define EQUALS             '='
#define EOL_LINUX          '\n'
#define EOL_OSX            '\r'
#define SPACE              ' '

enum char_position {
    char_first = -1,
    char_last = 1,
};

static int pos_char_lr(const char what, const char buff[], size_t buff_size, enum char_position pos)
{
    int result = -1;

    assert(buff);

    char cur_char;
    int i = 0;

    while ((cur_char = buff[i]) != '\0') {
        if (i >= (int)buff_size) { break; }
        if (cur_char == what) {
            if (pos == char_last) {
                if (buff[i+1] != cur_char) {
                    break;
                } else {
                    continue;
                }
            } else {
                break;
            }
        }
        i++;
    }
    result = i;

    return result;
}

static long int first_pos_char(const char what, const char buff[], const size_t buff_size)
{
    return pos_char_lr(what, buff, buff_size, char_first);
}

static long int last_pos_char(const char what, const char buff[], const size_t buff_size)
{
    return pos_char_lr(what, buff, buff_size, char_last);
}

static int ini_parse(const char *buff, const size_t buff_size, struct ini_config *config)
{
    int result = -1;

    assert(buff);
    assert(config);

    long int pos = 0;
    char cur_char;
    struct ini_group *group = NULL;

    while ((cur_char = buff[pos]) != '\0') {
        if (pos >= (int)buff_size) { break; }

        const char *cur_buff = &buff[pos];

        const long int rem_buff_size = (long int)buff_size - pos;

        const long int line_len = first_pos_char(EOL_LINUX, cur_buff, (size_t)rem_buff_size);
        if (line_len < 0) { break; }

        // nothing special move to next char
        pos += line_len + 1;

        if (line_len == 0) { continue; }

        char line[1024] = {0};
        memcpy(line, cur_buff, (size_t)line_len);

        /* comment */
        if (cur_char == COMMENT_SEMICOLON || cur_char == COMMENT_HASH) { continue; }
        /* add new group */
        if (cur_char == GROUP_OPEN) {
            const long int grp_end_pos = first_pos_char(GROUP_CLOSE, line, (size_t)line_len);
            if (grp_end_pos <= 0) { continue; }

            const long int name_len = grp_end_pos - 1;
            char name[1024] = {0};
            memcpy(name, line + 1, (size_t)name_len);

            group = ini_group_new(name);
            ini_config_append_group(config, group);
        }
        if (NULL == group) {
            // if there isn't a group we create a default one
            group = ini_group_new(DEFAULT_GROUP_NAME);
            ini_config_append_group(config, group);
        }
        /* add new key = value pair to current group */
        assert(NULL != group);

        const long int equal_pos = first_pos_char(EQUALS, line, (size_t)line_len);

        struct grrr_string *key_str = _grrrs_new_empty(1024);
        key_str->len = (uint32_t)equal_pos;
        memcpy(key_str->data, line, key_str->len);
        grrrs_trim(key_str->data, NULL);

        long int val_pos = equal_pos;
        long int val_len = (long int)line_len - equal_pos - 1; // subtract the eol

        const long int n_space_pos = last_pos_char(SPACE, line + val_pos, (size_t)line_len - key_str->len);

        if (n_space_pos >= 0) {
            val_pos += n_space_pos + 1;
            val_len -= n_space_pos;
        }
        if (val_len > 0) {
            struct grrr_string *val_str = _grrrs_new_empty(1024);
            val_str->len = (uint32_t)line_len;
            memcpy(val_str->data, line+val_pos, val_str->len);
            grrrs_trim(val_str->data, NULL);

            struct ini_value *value = ini_value_new(key_str->data, val_str->data);
            ini_group_append_value(group, value);
            free(val_str);
        }
        free(key_str);
        if (result < 0) {
            result = 0;
        }
        result++;
    }

    return result;
}

#endif // MPRIS_SCROBBLER_INI_H
