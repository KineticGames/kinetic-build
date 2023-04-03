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
