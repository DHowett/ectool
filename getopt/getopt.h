#pragma once

#include "optparser.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool _optreset;
extern int optind;
extern char* optarg;
int getopt_long(int argc, char** argv, char* optstring, const struct option opts[], int* longindex);

#ifdef __cplusplus
}
#endif
