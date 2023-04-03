#include "dependencies.h"

#include "directory.h"

// std
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PATH 512
#define MAX_COMMAND 1536

bool clone_dependencies_to(const char *path, kinetic_project project) {
  bool success = true;

  for (size_t i = 0; i < project.dependency_count; ++i) {
    char clone_path[MAX_PATH];
    snprintf(clone_path, MAX_PATH, "%s/%s-%zu_%zu_%zu", path,
             project.dependencies[i].name,
             project.dependencies[i].version.major,
             project.dependencies[i].version.minor,
             project.dependencies[i].version.patch);
    if (!directory_exists(clone_path)) {
      char command[MAX_COMMAND];
      snprintf(command, MAX_COMMAND, "git clone -q -- %s %s\n",
               project.dependencies[i].url, clone_path);

      printf("Downloading: %s\n", project.dependencies[i].name);

      if (system(command) != 0) {
        fprintf(stderr, "Download failed: %s\n", command);
        return false;
      };
    }

    kinetic_project dependency_project;
    if (!kinetic_project_from_dir(clone_path, &dependency_project)) {
      fprintf(stderr, "Dependency %s is not a kinetic project!",
              project.dependencies[i].name);
      success = false;
      continue;
    }

    success &= clone_dependencies_to(path, dependency_project);
  }

  return success;
}
