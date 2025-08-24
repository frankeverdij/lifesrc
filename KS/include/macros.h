#ifndef MACROS_H
#define MACROS_H

#include <stdio.h>

#define FATAL(...)  fprintf(stderr,__VA_ARGS__),exit(EXIT_FAILURE)

#endif /* MACROS_H */
