#ifndef KINETIC_BUILD_PROJECT_DEFINITION_H
#define KINETIC_BUILD_PROJECT_DEFINITION_H

// lib
#include <kinetic_notation.h>

// std
#include <stdbool.h>
#include <stddef.h>

#define NAME_KEY "name"
#define VERSION_KEY "version"

#define LIBRARY_KEY "lib"
#define LIBRARY_SHARED_KEY "shared"

#define DEPENDENCIES_KEY "dependencies"
#define DEPENDENCY_NAME_KEY "name"
#define DEPENDENCY_VERSION_KEY "version"
#define DEPENDENCY_URL_KEY "url"

typedef struct {
  char *project_dir;
  char *name;
  kn_version version;
  bool is_lib;
  struct {
    bool shared;
  } lib;
  size_t dependency_count;
  struct dependency {
    char *name;
    kn_version version;
    char *url;
  } *dependencies;
} kinetic_project;

kn_definition *kinetic_project_create_definition();

bool kinetic_project_from_dir(char *dir_path, kinetic_project *project);

void kinetic_project_clear(kinetic_project *project);

#endif // KINETIC_BUILD_PROJECT_DEFINITION_H
