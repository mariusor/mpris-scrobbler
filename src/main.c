/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "slastfm.h"
#include "sutils.h"

int main (int argc, char** argv)
{
    char* command = 0;
    if (argc > 0) {
        command = argv[1];
    }
    if (command == 0) {
    }
    lastfm_credentials *credentials = load_credentials();
    if (!credentials) {
        _log(error, "Unable to load credentials");
        return EXIT_FAILURE;
    }

    free_credentials(credentials);
    return EXIT_SUCCESS;
}

