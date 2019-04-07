/**
 *
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SSTRINGS_H
#define MPRIS_SCROBBLER_SSTRINGS_H

int string_trim_left(char **s, int len)
{
    if (NULL == s) { return -1; }
    if (NULL == *s) { return -1; }
    int new_len = len;

    for (int i = 0; i <= len; i++) {
        char c = *((*s) + i);
        if (c == 0x0) {
            new_len--;
            break;
        }
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            *s +=  i;
            new_len--;
        } else {
            break;
        }
    }
    return new_len;
}

int string_trim_right(char **s, int len)
{
    if (NULL == s) { return -1; }
    if (NULL == *s) { return -1; }
    int new_len = len;

    for (int i = len - 1; i >= 0; i--) {
        char c = *((*s) + i);
        if (c == 0x0) {
            break;
        }
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            *((*s) + i) = 0x0;
            new_len--;
        } else {
            break;
        }
    }
    return new_len;
}

int string_trim(char **s, int len)
{
    if (NULL == s) { return -1; }
    if (NULL == *s) { return -1; }

    int new_len = len;
    new_len = string_trim_left(s, len);
    new_len = string_trim_right(s, new_len);

    return new_len;
}


#endif // MPRIS_SCROBBLER_SSTRINGS_H
