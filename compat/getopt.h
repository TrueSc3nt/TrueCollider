/* Portable getopt for MSVC / environments without POSIX getopt.
 * Public-domain style minimal implementation.
 */
#ifndef COMPAT_GETOPT_H
#define COMPAT_GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

extern char *optarg;
extern int optind, opterr, optopt;

int getopt(int argc, char * const argv[], const char *optstring);

#ifdef __cplusplus
}
#endif

#endif
