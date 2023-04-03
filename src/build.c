#include "dependencies.h"
#include "directory.h"
#include "project_definition.h"

// lib
#include <kinetic_notation.h>

// std
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_INCLUDE 4096

#define TARGET_DIR_NAME "./target"
#define DEPENDENCY_CLONE_DIR "./target/deps"

typedef struct compile_command {
  char *directory;
  char *command;
  char *file_path;
  struct compile_command *next;
} compile_command;

char *get_name() { return "build"; }

int execute(int argc, char *argv[]) {
  kinetic_project project = {0};
  if (!kinetic_project_from_dir(".", &project)) {
    return 1;
  }

  create_dir(TARGET_DIR_NAME);

  clone_dependencies_to(DEPENDENCY_CLONE_DIR, project);

  // if (create_dir("target/deps") == false) {
  //   fprintf(stderr, "Could not create directory: \"target/deps\"\n");
  //   return 1;
  // }

  // for (size_t i = 0; i < project.dependency_count; ++i) {
  //   // clone_dep(&project.dependencies[i]);
  // }

  // if (!directory_exists("./src")) {
  //   fprintf(stderr, "Could not find 'src' directory\n");
  //   return 1;
  // }

  // if (!get_compile_commands("./src", realpath("./src", NULL), &project))
  // {
  //   fprintf(stderr, "Failed to get compile_commands\n");
  //   return 1;
  // }

  // for (size_t i = 0; i < project.dependency_count; ++i) {
  //   char source_path[MAX_PATH + 16];
  //   snprintf(source_path, MAX_PATH + 16, "%s/src",
  //            project.dependencies[i].path);

  //  if (!get_compile_commands(source_path, source_path, &project)) {
  //    fprintf(stderr, "Failed to get compile_commands for dependency:
  //    %s\n",
  //            project.dependencies[i].name);
  //    free(buffer);
  //    kn_definition_destroy(definition);
  //    free(project.dependencies);
  //    return 1;
  //  }
  //}

  // if (create_dir("target/build") == false) {
  //   fprintf(stderr, "Could not create directory: \"target/build\"\n");
  //   free(buffer);
  //   kn_definition_destroy(definition);
  //   return 1;
  // }

  // if (!create_compile_commands_json("compile_commands.json", project)) {
  //   fprintf(stderr, "Could not generate 'compile_commands.json'");
  // }

  //{
  //  compile_command *cc = project.compile_commands;
  //  while (cc != NULL) {
  //    if (system(cc->command) != 0) {
  //      fprintf(stderr, "Failed while running: %s\n", cc->command);
  //    }
  //    cc = cc->next;
  //  }
  //}

  // if (!link_objects(project)) {
  //   fprintf(stderr, "Failed to link\n");
  // }

  kinetic_project_clear(&project);
  return 0;
}

// static bool link_objects(kinetic_project project) {
//   char link_command[MAX_COMMAND];
//   if (project.is_lib) {
//     if (project.lib.shared) {
//       snprintf(link_command, MAX_COMMAND,
//                "/usr/bin/cc -shared -o %s/target/lib%s.so
//                %s/target/build/*.o", project.project_dir, project.name,
//                project.project_dir);
//     } else {
//       snprintf(link_command, MAX_COMMAND,
//                "/usr/bin/ar rcs %s/target/%s %s/target/build/*.o",
//                project.project_dir, project.name, project.project_dir);
//     }
//   } else {
//     snprintf(link_command, MAX_COMMAND,
//              "/usr/bin/cc -std=gnu11 -o %s/target/lib%s.a
//              %s/target/build/*.o", project.project_dir, project.name,
//              project.project_dir);
//   }
//
//   if (system(link_command) != 0) {
//     return false;
//   }
//
//   return true;
// }

// static bool create_compile_commands_json(const char *path,
//                                          kinetic_project project) {
//   FILE *fp = fopen(path, "w");
//   if (fp == NULL) {
//     fclose(fp);
//     return false;
//   }
//
//   fprintf(fp, "[");
//
//   compile_command *cc = project.compile_commands;
//
//   while (cc != NULL) {
//     fprintf(
//         fp,
//         "\n{\n\t\"directory\": \"%s\",\n\t\"command\": \"%s\",\n\t\"file\": "
//         "\"%s\"\n}",
//         cc->directory, cc->command, cc->file_path);
//     if (cc->next != NULL) {
//       fprintf(fp, ",");
//     }
//     cc = cc->next;
//   }
//
//   fprintf(fp, "\n]");
//
//   return true;
// }

// static bool get_compile_commands(const char *path, const char *src_path,
//                                  kinetic_project *project) {
//   DIR *dirp;
//   if ((dirp = opendir(path)) == NULL) {
//     fprintf(stderr, "Could not find path: %s\n", path);
//     return false;
//   }
//
//   struct dirent *entry;
//
//   while ((entry = readdir(dirp)) != NULL) {
//     if (entry->d_type == DT_REG) {
//       if (entry->d_name[strlen(entry->d_name) - 1] != 'c') {
//         continue;
//       }
//
//       char *dir_path = realpath(path, NULL);
//       char file_path[MAX_PATH];
//       snprintf(file_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);
//
//       char include[MAX_INCLUDE] = "";
//       if (project->include_dir) {
//         snprintf(include, MAX_INCLUDE, "-I%s/include", project->project_dir);
//       }
//
//       for (size_t i = 0; i < project->dependency_count; ++i) {
//         char new_include[MAX_PATH + 2];
//         snprintf(new_include, MAX_PATH + 2, " -I%s",
//                  project->dependencies[i].include_path);
//         size_t x = MAX_INCLUDE - strlen(include);
//         strncat(include, new_include, x);
//       }
//
//       size_t compile_flags_length = strlen(project->compile_flags) + 7;
//       char compile_flags[compile_flags_length];
//       strcpy(compile_flags, project->compile_flags);
//       if (project->is_lib && project->lib.shared) {
//         strcat(compile_flags, " -fPIC ");
//       }
//
//       char command[MAX_COMMAND];
//       snprintf(command, MAX_COMMAND,
//                "/usr/bin/cc %s -I%s %s -std=gnu11 -o "
//                "target/build/%s.o -c %s",
//                include, src_path, compile_flags, entry->d_name, file_path);
//
//       if (project->compile_commands == NULL) {
//         project->compile_commands = malloc(sizeof(compile_command));
//         project->compile_commands->directory = strdup(dir_path);
//         project->compile_commands->file_path = strdup(file_path);
//         project->compile_commands->command = strdup(command);
//         project->compile_commands->next = NULL;
//       }
//
//       compile_command *cc = project->compile_commands;
//
//       while (cc->next != NULL) {
//         cc = cc->next;
//       }
//
//       cc->next = malloc(sizeof(compile_command));
//       cc->next->directory = strdup(dir_path);
//       cc->next->file_path = strdup(file_path);
//       cc->next->command = strdup(command);
//       cc->next->next = NULL;
//
//       free(dir_path);
//
//     } else if (entry->d_type == DT_DIR) {
//       if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") ==
//       0) {
//         continue;
//       }
//       char dir_path[MAX_PATH];
//       snprintf(dir_path, MAX_PATH, "%s/%s", path, entry->d_name);
//
//       if (!get_compile_commands(dir_path, path, project)) {
//         fprintf(stderr, "Could not get files in %s\n", dir_path);
//         closedir(dirp);
//         return false;
//       }
//     }
//   }
//
//   closedir(dirp);
//   return true;
// }

// static bool get_dependency(kn_definition *definition, dependency *dependency)
// {
//   if (kn_definition_get_string(definition, DEPENDENCY_NAME_KEY,
//                                &dependency->name) != SUCCESS) {
//     return false;
//   }
//
//   if (kn_definition_get_version(definition, DEPENDENCY_VERSION_KEY,
//                                 &dependency->version) != SUCCESS) {
//     return false;
//   }
//
//   if (kn_definition_get_string(definition, DEPENDENCY_URL_KEY,
//                                &dependency->url) != SUCCESS) {
//     return false;
//   }
//
//   return true;
// }
