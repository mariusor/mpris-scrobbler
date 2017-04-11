/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
typedef struct lastfm_credentials {
    char* user_name;
    char* password;
} lastfm_credentials;

lastfm_credentials* free_credentials(lastfm_credentials *credentials) {
    if (credentials) {
        if (credentials->user_name) {
            free(credentials->user_name);
        }
        if (credentials->password) {
            free(credentials->password);
        }
        free(credentials);
    }
    return NULL;
}

