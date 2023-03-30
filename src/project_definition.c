#include "project_definition.h"

#include "defer.h"
#include "kinetic_notation/definition.h"

// lib
#include <kinetic_notation.h>

// std
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define file_ops(file, ops)                                                    \
  FILE *fp;                                                                    \
  defer(fp = fopen(file, ops), fclose(fp))

static long get_file_length(FILE *fp);
static char *read_file_to_buffer(char *path);
static kn_definition *read_project_file(char *path);

kn_definition *kinetic_project_create_definition() {
  kn_definition *definition = kn_definition_new();

  bool result = true;
  result &= kn_definition_add_string(definition, NAME_KEY);
  result &= kn_definition_add_version(definition, VERSION_KEY);

  kn_definition *lib_definition = kn_definition_new();
  result &= kn_definition_add_boolean(lib_definition, LIBRARY_SHARED_KEY);
  result &= kn_definition_add_object(definition, LIBRARY_KEY, lib_definition);

  kn_definition *dependency_outline = kn_definition_new();
  result &= kn_definition_add_string(dependency_outline, DEPENDENCY_NAME_KEY);
  result &=
      kn_definition_add_version(dependency_outline, DEPENDENCY_VERSION_KEY);
  result &= kn_definition_add_string(dependency_outline, DEPENDENCY_URL_KEY);
  result &= kn_definition_add_object_array(definition, DEPENDENCIES_KEY,
                                           dependency_outline);

  if (!result) {
    kn_definition_destroy(definition);
    return NULL;
  }

  return definition;
}

bool kinetic_project_from_dir(char *dir_path, kinetic_project *project) {
  project->project_dir = realpath(dir_path, NULL);

  // Build the path to the project file
  size_t project_file_path_length = strlen(project->project_dir) + 12;
  char project_file_path[project_file_path_length];
  snprintf(project_file_path, project_file_path_length, "%s/kinetic.kn",
           project->project_dir);

  kn_definition *project_definition = read_project_file(project_file_path);

  if (kn_definition_get_string(project_definition, NAME_KEY, &project->name) !=
      SUCCESS) {
    return false;
  }

  if (kn_definition_get_version(project_definition, VERSION_KEY,
                                &project->version) != SUCCESS) {
    return false;
  }

  kn_definition *lib_definition;
  switch (kn_definition_get_object(project_definition, LIBRARY_KEY,
                                   &lib_definition)) {
  case SUCCESS:
    switch (kn_definition_get_boolean(lib_definition, LIBRARY_SHARED_KEY,
                                      &project->lib.shared)) {
    case SUCCESS:
      break;
    case NOT_FILLED_IN:
      project->lib.shared = false;
      break;
    default:
      return false;
    }
    break;
  case NOT_FILLED_IN:
    project->is_lib = false;
    break;
  default:
    return false;
  }

  if (kn_definition_get_object_array_length(
          project_definition, DEPENDENCIES_KEY, &project->dependency_count) !=
      SUCCESS) {
    return false;
  }

  project->dependencies =
      calloc(project->dependency_count, sizeof(struct dependency));

  for (size_t i = 0; i < project->dependency_count; ++i) {
    kn_definition *dependency_definition;
    if (kn_definition_get_object_from_array_at_index(
            project_definition, DEPENDENCIES_KEY, i, &dependency_definition) !=
        SUCCESS) {
      return false;
    }

    if (kn_definition_get_string(dependency_definition, DEPENDENCY_NAME_KEY,
                                 &project->dependencies[i].name) != SUCCESS) {
      return false;
    }

    if (kn_definition_get_version(dependency_definition, DEPENDENCY_VERSION_KEY,
                                  &project->dependencies[i].version) !=
        SUCCESS) {
      return false;
    }

    if (kn_definition_get_string(dependency_definition, DEPENDENCY_URL_KEY,
                                 &project->dependencies[i].url) != SUCCESS) {
      return false;
    }
  }

  kn_definition_destroy(project_definition);

  return true;
}

void kinetic_project_clear(kinetic_project *project) {
  free(project->project_dir);
  free(project->name);
  for (size_t i = 0; i < project->dependency_count; ++i) {
    free(project->dependencies[i].name);
    free(project->dependencies[i].url);
    free(project->dependencies[i].path);
  }
  free(project->dependencies);
  memset(project, 0, sizeof(kinetic_project));
}

static long get_file_length(FILE *fp) {
  if (fp == NULL) {
    return 0;
  }

  long stored_offset = ftell(fp);

  if (fseek(fp, 0L, SEEK_END) != 0) {
    return -1;
  }

  long file_length = ftell(fp);

  if (fseek(fp, stored_offset, SEEK_SET) != 0) {
    return -1;
  }

  return file_length;
}

static char *read_file_to_buffer(char *path) {
  char *buffer;

  file_ops(path, "r") {
    if (fp == NULL) {
      return NULL;
    }

    long file_length = get_file_length(fp);
    if (file_length < 0) {
      fclose(fp);
      return NULL;
    }

    buffer = malloc(sizeof(char) * file_length + 1);

    fread(buffer, sizeof(char), file_length, fp);
    if (ferror(fp) != 0) {
      fclose(fp);
      return NULL;
    }

    buffer[file_length++] = '\0';
  }

  return buffer;
}

/// If it returns NULL it might be useful to check kinetic_notation_get_error()
static kn_definition *read_project_file(char *path) {
  char *buffer = read_file_to_buffer(path);
  if (buffer == NULL) {
    return NULL;
  }

  kn_definition *definition = kinetic_project_create_definition();
  if (definition == NULL) {
    free(buffer);
    return NULL;
  }

  if (!kinetic_notation_parse(definition, buffer)) {
    free(buffer);
    kn_definition_destroy(definition);
    return NULL;
  }

  free(buffer);

  return definition;
}
