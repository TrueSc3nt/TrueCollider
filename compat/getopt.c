#include "getopt.h"
#include <stdio.h>
#include <string.h>

char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = 0;

/*
 * Minimal getopt with GNU-style optional arguments (optchar followed by "::").
 * For optional args, the value must be attached (-Nhttp://...), never a separate argv.
 */
int getopt(int argc, char * const argv[], const char *optstring) {
    static int sp = 1;
    int c;
    const char *cp;

    if (sp == 1) {
        if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
            return -1;
        if (strcmp(argv[optind], "--") == 0) {
            optind++;
            return -1;
        }
    }
    optopt = c = argv[optind][sp];
    if (c == ':' || (cp = strchr(optstring, c)) == NULL) {
        if (opterr)
            fprintf(stderr, "%s: illegal option -- %c\n", argv[0], c);
        if (argv[optind][++sp] == '\0') {
            optind++;
            sp = 1;
        }
        return '?';
    }
    if (*++cp == ':') {
        int optional = (*(cp + 1) == ':');
        if (argv[optind][sp + 1] != '\0') {
            /* Attached argument: -Nhttp://... or -fFILE */
            optarg = &argv[optind++][sp + 1];
            sp = 1;
        } else if (optional) {
            /* Bare -N: no argument (do not consume next argv / next flag). */
            optarg = NULL;
            sp = 1;
            optind++;
        } else if (++optind >= argc) {
            if (opterr)
                fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0], c);
            sp = 1;
            return '?';
        } else {
            optarg = argv[optind++];
            sp = 1;
        }
    } else {
        if (argv[optind][++sp] == '\0') {
            sp = 1;
            optind++;
        }
        optarg = NULL;
    }
    return c;
}
