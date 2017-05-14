/*
  Maciej Buszka
  279129
*/

#ifndef UTIL_H
#define UTIL_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define panic(msg) do { \
  fprintf(stderr, "%s : %s/n", __func__, msg); \
  exit(1); \
} while(0)

#define fail(err_number) do {\
  fprintf(stderr, "%s : %s\n", __func__, strerror(err_number));\
  exit(1);\
} while (0)

#define min(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define max(_a, _b) ((_a) > (_b) ? (_a) : (_b))

#endif
