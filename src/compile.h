#ifndef KINETIC_BUILD_COMPILE_H
#define KINETIC_BUILD_COMPILE_H

#include "project_definition.h"

// std
#include <stddef.h>

typedef struct {
  char *directory;
  char *command;
  char *file;
} compile_command;

typedef struct {
  size_t command_count;
  compile_command *commands;
} compile_commands;

compile_commands get_compile_commands_for_directory(const char *path,
                                                    const char *target_path,
                                                    kinetic_project project);
void combine_compile_commands(compile_commands *dest, compile_commands source);
bool compile_dependencies(const char *clone_dir, const char *build_dir);
bool generate_compile_commands_json(compile_commands commands);
bool run_commands(compile_commands commands);

#endif // KINETIC_BUILD_COMPILE_H
