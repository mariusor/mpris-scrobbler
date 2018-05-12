/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_INI_H
#define MPRIS_SCROBBLER_INI_H

#include "ini_base.h"

#define COMMENT_MARK    ';'
#define GROUP_OPEN      '['
#define GROUP_CLOSE     ']'
#define EQUALS          '='
#define EOL_LINUX       '\n'
#define EOL_OSX         '\r'
#define SPACE           ' '

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

static int first_pos_char(const char what, const char buff[], size_t buff_size)
{
    return pos_char_lr(what, buff, buff_size, char_first);
}

static int last_pos_char(const char what, const char buff[], size_t buff_size)
{
    return pos_char_lr(what, buff, buff_size, char_last);
}

int ini_parse(const char buff[], size_t buff_size, struct ini_config *config)
{
    int result = -1;

    assert(buff);
    assert(config);

    int i = 0;
    char cur_char;
    struct ini_group *group = NULL;

    while ((cur_char = buff[i]) != '\0') {
        if (i >= (int)buff_size) { break; }

        const char *cur_buff = &buff[i];

        int rem_buff_size = buff_size - i;

        int line_len = first_pos_char(EOL_LINUX, cur_buff, rem_buff_size);
        if (line_len < 0) { break; }

        // nothing special move to next char
        i += (line_len + 1);

        if (line_len == 0) { continue; }

        char *line = calloc(line_len + 1, sizeof(char));
        strncpy(line, (char*)cur_buff, line_len + 1);

        /* comment */
        if (cur_char == COMMENT_MARK) { goto __continue; }
        /* add new group */
        if (cur_char == GROUP_OPEN) {
            int grp_end_pos = first_pos_char(GROUP_CLOSE, line, line_len);
            if (grp_end_pos <= 0) { goto __continue; }

            int name_len = grp_end_pos - 1;
            char *name = calloc(name_len + 1, sizeof(char));
            strncpy(name, line + 1, name_len);

            group = ini_group_new(name);

            ini_config_append_group(config, group);
            free(name);
            goto __continue;
        }
        /* add new key = value pair to current group */
        assert(NULL != group);

        int equal_pos = first_pos_char(EQUALS, line, line_len);
        char *val_line = line + equal_pos;

        int key_len = equal_pos - 1;
        char *key_str = calloc(key_len + 1, sizeof(char));
        strncpy(key_str, line, key_len);

        int n_space_pos = last_pos_char(SPACE, val_line, line_len - key_len);

        int val_len = line_len - equal_pos - 1; // subtract the eol too
        if (n_space_pos > 0) {
            val_line = line + equal_pos + n_space_pos;
            val_len -= n_space_pos;
        }
        char *val_str = calloc(val_len + 1, sizeof(char));
        strncpy(val_str, val_line + 1, val_len);

        struct ini_value *value = ini_value_new(key_str, val_str);
        ini_group_append_value(group, value);

        free(val_str);
        free(key_str);

__continue:
        free(line);
    }

    return result;
}

#endif // MPRIS_SCROBBLER_INI_H
