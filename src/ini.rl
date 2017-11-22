#ifndef MPRIS_SCROBBLER_INI_H
#define MPRIS_SCROBBLER_INI_H
// -*-c-*-

#include "ini_base.h"

%%{

    machine ini;

    action mark { mark = fpc - data; }

    action store_group {
        memset(group_name, 0, MAX_LENGTH);
        ptrdiff_t end_mark  = fpc - data;
        strncpy(group_name, data + mark, (size_t)end_mark - mark);

        group =  ini_group_new(group_name);
    }

    action store_key {
        memset(key, 0, MAX_LENGTH);
        ptrdiff_t end_mark  = fpc - data;
        strncpy(key, data + mark, (size_t)end_mark - mark);
    }

    action store_value {
        memset(value, 0, MAX_LENGTH);
        ptrdiff_t end_mark  = fpc - data;
        strncpy(value, data + mark, (size_t)end_mark - mark);
    }

    action append_value {
        ini_value *v = ini_value_new(key, value);
        ini_group_append_value(group, v);
    }

    action append_group {
        ini_config_append_group(conf, group);
        group = NULL;
    }

    eolc = ( '\r' | '\n' );
    eol = ( eolc | '\r\n' );
    ws = ( ' ' | '\t' )+;
    string = ( alpha | digit | '-' | '_' )+;
    value_string = (any -- eolc)+;
    key   = string >mark %store_key;
    value = value_string >mark %store_value;

    comment = ( '#' | ';' ) value_string;
    group   = '[' >append_group string >mark %store_group ']';
    setting = key ws '=' ws value %append_value;

    line = ( comment | group | setting ) {,1} eol;

    main := line* %append_group;

    write data;
}%%

ini_config *ini_load(char *data)
{

    char *p = data;
    char *pe = (char*)data + strlen((char*)data);
    char *eof = pe;
    ptrdiff_t mark;
    int cs;

    char *group_name = calloc(1, MAX_LENGTH);
    char *value = calloc(1, MAX_LENGTH);
    char *key = calloc(1, MAX_LENGTH);

    ini_group *group = NULL;
    ini_config *conf = ini_config_new();

    %% write init;

    %% write exec;

    free(group_name);
    free(group);
    free(value);
    free(key);

    return conf;
}

#endif // MPRIS_SCROBBLER_INI_H
