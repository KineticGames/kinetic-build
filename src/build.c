#include <kinetic_notation.h>

// std
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define NAME_KEY "name"
#define VERSION_KEY "version"

#define LIBRARY_KEY "lib"
#define LIBRARY_SHARED_KEY "shared"

#define DEPENDENCIES_KEY "dependencies"
#define DEPENDENCY_NAME_KEY "name"
#define DEPENDENCY_VERSION_KEY "version"
#define DEPENDENCY_URL_KEY "url"

typedef struct {
  char *name;
  kn_version version;
  char *url;
} dependency;

typedef struct {
  char *name;
  kn_version version;
  bool is_lib;
  struct {
    bool shared;
  } lib;
  size_t dependency_count;
  dependency *dependencies;
} project;

static bool create_dir(const char *dir_name);
static char *read_file_to_buffer(const char *file_path);
static kn_definition *create_definition();
static bool get_dependency(kn_definition *definition, dependency *dependency);
static bool get_project(kn_definition *definition, project *project);

char *get_name() { return "build"; }

int execute(int argc, char *argv[]) {
  if (create_dir("target") == false) {
    fprintf(stderr, "Could not create directory: \"target\"\n");
    return 1;
  }

  char *buffer = read_file_to_buffer("kinetic.kn");
  if (buffer == NULL) {
    fprintf(stderr, "Could not read file \"kinetic.kn\"\n");
    free(buffer);
    return 1;
  }

  kn_definition *definition = create_definition();
  if (kinetic_notation_parse(definition, buffer) == false) {
    fprintf(stderr, "Parsing error: %s\n", kinetic_notation_get_error());
    free(buffer);
    kn_definition_destroy(definition);
    return 1;
  }

  project project;
  if (get_project(definition, &project) == false) {
    free(buffer);
    kn_definition_destroy(definition);
    return 1;
  }

  free(buffer);
  kn_definition_destroy(definition);
  free(project.dependencies);
  return 0;
}

static bool get_project(kn_definition *definition, project *project) {
  struct get_string_result name_result =
      kn_definition_get_string(definition, NAME_KEY);
  if (name_result.result != SUCCESS) {
    fprintf(stderr, "Could not get project name\n");
    return false;
  }
  project->name = name_result.string;

  struct get_version_result version_result =
      kn_definition_get_version(definition, VERSION_KEY);
  if (version_result.result != SUCCESS) {
    fprintf(stderr, "Could not get project version: %d\n",
            version_result.result);
    return false;
  }
  project->version = version_result.version;

  struct get_object_result lib_result =
      kn_definition_get_object(definition, LIBRARY_KEY);
  if (lib_result.result != SUCCESS && lib_result.result != NOT_FILLED_IN) {
    fprintf(stderr, "Could not get lib\n");
    return false;
  } else if (lib_result.result == NOT_FILLED_IN) {
    project->is_lib = false;
  } else {
    project->is_lib = true;

    struct get_boolean_result lib_shared_result =
        kn_definition_get_boolean(lib_result.object, NAME_KEY);
    if (lib_shared_result.result == NOT_FILLED_IN) {
      project->lib.shared = false;
    } else if (lib_shared_result.result != SUCCESS) {
      fprintf(stderr, "Could not get lib.shared\n");
      return false;
    }
    project->lib.shared = lib_shared_result.boolean;
  }

  struct get_object_array_length_result length_result =
      kn_definition_get_object_array_length(definition, DEPENDENCIES_KEY);
  if (length_result.result != NOT_FILLED_IN) {
    project->dependency_count = 0;
    project->dependencies = NULL;
  } else if (length_result.result != SUCCESS) {
    fprintf(stderr, "Could not get dependency count\n");
    return false;
  } else {
    size_t length = length_result.length;
    project->dependency_count = length;
    project->dependencies = calloc(length, sizeof(dependency));

    for (size_t i = 0; i < length; ++i) {
      struct get_object_at_index_result dependency_result =
          kn_definition_get_object_from_array_at_index(definition,
                                                       DEPENDENCIES_KEY, i);
      if (dependency_result.result == SUCCESS) {
        get_dependency(dependency_result.object, &project->dependencies[i]);
      } else {
        fprintf(stderr, "Could not get dependency %zu\n", i);
        return false;
      }
    }
  }

  return true;
}

static bool get_dependency(kn_definition *definition, dependency *dependency) {
  struct get_string_result name_result =
      kn_definition_get_string(definition, DEPENDENCY_NAME_KEY);
  if (name_result.result != SUCCESS) {
    return false;
  }
  (*dependency).name = name_result.string;

  struct get_version_result version_result =
      kn_definition_get_version(definition, DEPENDENCY_VERSION_KEY);
  if (version_result.result != SUCCESS) {
    return false;
  }
  (*dependency).version = version_result.version;

  struct get_string_result url_result =
      kn_definition_get_string(definition, DEPENDENCY_NAME_KEY);
  if (url_result.result != SUCCESS) {
    return false;
  }
  (*dependency).name = url_result.string;

  return true;
}

static kn_definition *create_definition() {
  kn_definition *definition = kn_definition_new();

  kn_definition_add_string(definition, NAME_KEY);
  kn_definition_add_version(definition, VERSION_KEY);

  kn_definition *lib_definition = kn_definition_new();
  kn_definition_add_boolean(lib_definition, LIBRARY_SHARED_KEY);
  kn_definition_add_object(definition, LIBRARY_KEY, lib_definition);

  kn_definition *dependency_definition = kn_definition_new();
  kn_definition_add_string(dependency_definition, DEPENDENCY_NAME_KEY);
  kn_definition_add_version(dependency_definition, DEPENDENCY_VERSION_KEY);
  kn_definition_add_string(dependency_definition, DEPENDENCY_URL_KEY);
  kn_definition_add_object_array(definition, DEPENDENCIES_KEY,
                                 dependency_definition);

  return definition;
}

static char *read_file_to_buffer(const char *file_path) {
  FILE *fp = fopen(file_path, "r");
  if (fp == NULL) {
    return NULL;
  }

  if (fseek(fp, 0L, SEEK_END) != 0) {
    fclose(fp);
    return NULL;
  }

  long buffer_size = ftell(fp);
  if (buffer_size == -1) {
    fclose(fp);
    return NULL;
  }

  char *buffer = malloc(sizeof(char) * (buffer_size + 1));

  if (fseek(fp, 0L, SEEK_SET) != 0) {
    fclose(fp);
    return NULL;
  }

  size_t new_length = fread(buffer, sizeof(char), buffer_size, fp);
  if (ferror(fp) != 0) {
    fclose(fp);
    return NULL;
  }

  buffer[new_length++] = '\0';

  fclose(fp);

  return buffer;
}

static bool create_dir(const char *dir_name) {
  DIR *dir;
  if ((dir = opendir(dir_name)) == NULL) {
    if (ENOENT == errno) {
      mkdir("build", 0777);
    } else {
      return false;
    }
  }

  return true;
}
