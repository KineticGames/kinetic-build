#ifndef KINETIC_BUILD_COMPILE_H
#define KINETIC_BUILD_COMPILE_H

#include "project_definition.h"

// std
#include <stddef.h>

typedef struct {
  size_t command_count;
  char **commands;
} compile_commands;

compile_commands get_compile_commands_for_directory(const char *path,
                                                    const char *target_path,
                                                    kinetic_project project);
void combine_compile_commands(compile_commands *dest, compile_commands source);

#endif // KINETIC_BUILD_COMPILE_H
