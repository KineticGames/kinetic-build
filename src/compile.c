#include "compile.h"

#include "directory.h"

// std
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif // MAX_PATH

#ifndef MAX_INCLUDE
#define MAX_INCLUDE 6144
#endif // MAX_INCLUDE

#ifndef MAX_COMMAND
#define MAX_COMMAND 8192
#endif // MAX_COMMAND

#ifndef COMPILE_FLAGS
#define COMPILE_FLAGS "-g -Wall -Wextra"
#endif // COMPILE_FLAGS

struct command {
  char *command;
  struct command *next;
};

compile_commands get_compile_commands_for_directory(const char *path,
                                                    const char *target_path,
                                                    kinetic_project project) {
  if (!directory_exists(path)) {
    fprintf(stderr, "Could not find directory: '%s'", path);
    return (compile_commands){.command_count = 0, .commands = NULL};
  }

  DIR *dir;
  if ((dir = opendir(path)) == NULL) {
    fprintf(stderr, "Could not open directory: '%s'", path);
    return (compile_commands){.command_count = 0, .commands = NULL};
  }

  char includes[MAX_INCLUDE] = "";
  char project_include_dir[strlen(project.project_dir) + strlen("/include")];
  sprintf(project_include_dir, "%s/include", project.project_dir);
  if (directory_exists(project_include_dir)) {
    snprintf(includes, MAX_INCLUDE, "-I%s", project_include_dir);
  }

  for (size_t i = 0; i < project.dependency_count; ++i) {
    char new_include[MAX_PATH];
    snprintf(new_include, MAX_PATH, " -I%s/deps/%s/include",
             project.project_dir, project.dependencies[i].name);
    size_t x = MAX_INCLUDE - strlen(includes) - 1;
    strncat(includes, new_include, x);
  }

  size_t compile_flags_length = strlen(COMPILE_FLAGS) + 7;
  char compile_flags[compile_flags_length];
  strcpy(compile_flags, COMPILE_FLAGS);
  if (project.is_lib && project.lib.shared) {
    strcat(compile_flags, " -fPIC ");
  }

  compile_commands commands = {0};

  struct command *first_command = NULL;
  struct command *command = NULL;
  size_t command_count = 0;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    switch (entry->d_type) {
    case DT_REG: {
      const char *dot = strrchr(entry->d_name, '.');
      if (!dot || *(dot + sizeof(char)) != 'c' ||
          *(dot + 2 * sizeof(char)) != '\0') {
        continue;
      }

      size_t name_len = entry->d_name - dot;
      char file_name_woe[name_len];
      snprintf(file_name_woe, name_len, "%s", entry->d_name);

      if (first_command == NULL) {
        first_command = calloc(1, sizeof(struct command));
        command = first_command;
      } else {
        command->next = calloc(1, sizeof(struct command));
        command = command->next;
      }

      command->command = calloc(MAX_COMMAND, sizeof(char));
      snprintf(command->command, MAX_COMMAND,
               "/usr/bin/cc %s -I%s/src %s -std=gnu11 -o "
               "%s/%s.o -c %s",
               includes, project.project_dir, compile_flags, target_path,
               file_name_woe, entry->d_name);

      command_count++;

      break;
    }
    case DT_DIR:
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      }

      char subdir_path[MAX_PATH];
      snprintf(subdir_path, MAX_PATH, "%s/%s", path, entry->d_name);

      char new_target_path[MAX_PATH];
      snprintf(new_target_path, MAX_PATH, "%s/%s", target_path, entry->d_name);

      compile_commands subdir_commands = get_compile_commands_for_directory(
          subdir_path, new_target_path, project);

      combine_compile_commands(&commands, subdir_commands);
      break;
    default:
      break;
    }
  }

  command = first_command;
  commands.command_count = command_count;
  commands.commands = calloc(command_count, sizeof(char *));
  for (size_t i = 0; i < command_count; ++i) {
    commands.commands[i] = command->command;

    struct command *next = command->next;
    free(command);
    command = next;
  }

  closedir(dir);
  return commands;
}

void combine_compile_commands(compile_commands *dest, compile_commands source) {
  size_t new_count = dest->command_count + source.command_count;

  dest->commands = reallocarray(dest->commands, new_count, sizeof(char *));
  memcpy(&dest->commands[dest->command_count], source.commands,
         source.command_count * sizeof(char *));

  dest->command_count = new_count;
}
