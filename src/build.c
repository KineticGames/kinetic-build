#include "compile.h"
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

#define TARGET_DIR "./target"
#define DEPENDENCY_CLONE_DIR "./target/deps"
#define BUILD_DIR "./target/build"

char *get_name() { return "build"; }

int execute(int argc, char *argv[]) {
  kinetic_project project = {0};
  if (!kinetic_project_from_dir(".", &project)) {
    return 1;
  }

  create_dir(TARGET_DIR);

  clone_dependencies_to(DEPENDENCY_CLONE_DIR, project);
  create_dir(BUILD_DIR);

  char project_build_dir[sizeof(BUILD_DIR) + sizeof(project.name) + 1];
  sprintf(project_build_dir, "%s/%s", BUILD_DIR, project.name);
  compile_commands project_compile_commands =
      get_compile_commands_for_directory("./src", project_build_dir, project);

  create_dir("target/build");

  compile_dependencies(DEPENDENCY_CLONE_DIR, BUILD_DIR);
  run_commands(project_compile_commands);

  generate_compile_commands_json(project_compile_commands);

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
