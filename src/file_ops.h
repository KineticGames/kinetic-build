#ifndef KINETIC_BUILD_FILE_OPS_H
#define KINETIC_BUILD_FILE_OPS_H

#include "defer.h"

// std
#include <stdio.h>

#define file_ops(file, ops)                                                    \
  FILE *fp;                                                                    \
  defer(fp = fopen(file, ops), fclose(fp))

#endif // KINETIC_BUILD_FILE_OPS_H
