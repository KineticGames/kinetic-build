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

#define NAME_KEY "name"
#define VERSION_KEY "version"
#define FLAGS_KEY "flags"

#define LIBRARY_KEY "lib"
#define LIBRARY_SHARED_KEY "shared"

#define DEPENDENCIES_KEY "dependencies"
#define DEPENDENCY_NAME_KEY "name"
#define DEPENDENCY_VERSION_KEY "version"
#define DEPENDENCY_URL_KEY "url"

#define MAX_INCLUDE 4096
#define MAX_COMMAND 8192
#define MAX_PATH 1024

typedef struct dependency {
  char *name;
  kn_version version;
  char *url;
  char *include_path;
  char *path;
} dependency;

typedef struct compile_command {
  char *directory;
  char *command;
  char *file_path;
  struct compile_command *next;
} compile_command;

typedef struct {
  char *name;
  kn_version version;
  char *compile_flags;
  bool is_lib;
  struct {
    bool shared;
  } lib;
  char *project_dir;
  size_t dependency_count;
  dependency *dependencies;
  bool include_dir;
  compile_command *compile_commands;
} project;

static bool create_dir(const char *dir_name);
static char *read_file_to_buffer(const char *file_path);
static kn_definition *create_definition();
static bool get_dependency(kn_definition *definition, dependency *dependency);
static bool get_project(kn_definition *definition, project *project);
static bool clone_dep(dependency *dependency);
static bool directory_exists(const char *path);
static bool get_compile_commands(const char *path, const char *src_path,
                                 project *project);
static bool create_compile_commands_json(const char *path, project project);
static bool link_objects(project project);

char *get_name() { return "build"; }

int execute(int argc, char *argv[]) {
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

  project.project_dir = realpath(".", NULL);

  if (create_dir("target") == false) {
    fprintf(stderr, "Could not create directory: \"target\"\n");
    free(buffer);
    kn_definition_destroy(definition);
    return 1;
  }

  if (create_dir("target/deps") == false) {
    fprintf(stderr, "Could not create directory: \"target/deps\"\n");
    free(buffer);
    kn_definition_destroy(definition);
    return 1;
  }

  for (size_t i = 0; i < project.dependency_count; ++i) {
    clone_dep(&project.dependencies[i]);
  }

  if (!directory_exists("./src")) {
    fprintf(stderr, "Could not find 'src' directory\n");
    free(buffer);
    kn_definition_destroy(definition);
    return 1;
  }

  project.include_dir = directory_exists("./include");
  project.compile_commands = NULL;

  if (!get_compile_commands("./src", realpath("./src", NULL), &project)) {
    fprintf(stderr, "Failed to get compile_commands\n");
    free(buffer);
    kn_definition_destroy(definition);
    free(project.dependencies);
    return 1;
  }

  for (size_t i = 0; i < project.dependency_count; ++i) {
    char source_path[MAX_PATH + 16];
    snprintf(source_path, MAX_PATH + 16, "%s/src",
             project.dependencies[i].path);

    if (!get_compile_commands(source_path, source_path, &project)) {
      fprintf(stderr, "Failed to get compile_commands for dependency: %s\n",
              project.dependencies[i].name);
      free(buffer);
      kn_definition_destroy(definition);
      free(project.dependencies);
      return 1;
    }
  }

  if (create_dir("target/build") == false) {
    fprintf(stderr, "Could not create directory: \"target/build\"\n");
    free(buffer);
    kn_definition_destroy(definition);
    return 1;
  }

  if (!create_compile_commands_json("target/compile_commands.json", project)) {
    fprintf(stderr, "Could not generate 'compile_commands.json'");
  }

  {
    compile_command *cc = project.compile_commands;
    while (cc != NULL) {
      if (system(cc->command) != 0) {
        fprintf(stderr, "Failed while running: %s\n", cc->command);
      }
      cc = cc->next;
    }
  }

  if (!link_objects(project)) {
    fprintf(stderr, "Failed to link\n");
  }

  free(buffer);
  kn_definition_destroy(definition);
  for (size_t i = 0; i < project.dependency_count; ++i) {
    free(project.dependencies[i].name);
    free(project.dependencies[i].url);
    free(project.dependencies[i].include_path);
  }
  free(project.name);
  free(project.compile_flags);
  free(project.dependencies);
  free(project.project_dir);
  compile_command *cc = project.compile_commands;
  while (cc != NULL) {
    compile_command *next = cc->next;
    free(cc->command);
    free(cc->directory);
    free(cc->file_path);
    free(cc);
    cc = next;
  }
  return 0;
}

static bool link_objects(project project) {
  char link_command[MAX_COMMAND];
  if (project.is_lib) {
    if (project.lib.shared) {
      snprintf(link_command, MAX_COMMAND,
               "/usr/bin/cc -shared -o %s/target/lib%s.so %s/target/build/*.o",
               project.project_dir, project.name, project.project_dir);
    } else {
      snprintf(link_command, MAX_COMMAND,
               "/usr/bin/ar rcs %s/target/%s %s/target/build/*.o",
               project.project_dir, project.name, project.project_dir);
    }
  } else {
    snprintf(link_command, MAX_COMMAND,
             "/usr/bin/cc -std=gnu11 -o %s/target/lib%s.a %s/target/build/*.o",
             project.project_dir, project.name, project.project_dir);
  }

  printf("Link command: %s\n", link_command);
  if (system(link_command) != 0) {
    return false;
  }

  return true;
}

static bool create_compile_commands_json(const char *path, project project) {
  FILE *fp = fopen(path, "w");
  if (fp == NULL) {
    fclose(fp);
    return false;
  }

  fprintf(fp, "[");

  compile_command *cc = project.compile_commands;

  while (cc != NULL) {
    fprintf(
        fp,
        "\n{\n\t\"directory\": \"%s\",\n\t\"command\": \"%s\",\n\t\"file\": "
        "\"%s\"\n}",
        cc->directory, cc->command, cc->file_path);
    if (cc->next != NULL) {
      fprintf(fp, ",");
    }
    cc = cc->next;
  }

  fprintf(fp, "\n]");

  return true;
}

static bool get_compile_commands(const char *path, const char *src_path,
                                 project *project) {
  DIR *dirp;
  if ((dirp = opendir(path)) == NULL) {
    fprintf(stderr, "Could not find path: %s\n", path);
    return false;
  }

  struct dirent *entry;

  while ((entry = readdir(dirp)) != NULL) {
    if (entry->d_type == DT_REG) {
      if (entry->d_name[strlen(entry->d_name) - 1] != 'c') {
        continue;
      }

      char *dir_path = realpath(path, NULL);
      char file_path[MAX_PATH];
      snprintf(file_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);

      char include[MAX_INCLUDE] = "";
      if (project->include_dir) {
        snprintf(include, MAX_INCLUDE, "-I%s/include", project->project_dir);
      }

      for (size_t i = 0; i < project->dependency_count; ++i) {
        char new_include[MAX_PATH + 2];
        snprintf(new_include, MAX_PATH + 2, " -I%s",
                 project->dependencies[i].include_path);
        size_t x = MAX_INCLUDE - strlen(include);
        strncat(include, new_include, x);
      }

      size_t compile_flags_length = strlen(project->compile_flags + 7);
      char compile_flags[compile_flags_length];
      strcpy(compile_flags, project->compile_flags);
      if (project->is_lib && project->lib.shared) {
        strcat(compile_flags, " -fPIC ");
      }

      char command[MAX_COMMAND];
      snprintf(command, MAX_COMMAND,
               "/usr/bin/cc %s -I%s %s -std=gnu11 -o "
               "target/build/%s.o -c %s",
               include, src_path, compile_flags, entry->d_name, file_path);

      if (project->compile_commands == NULL) {
        project->compile_commands = malloc(sizeof(compile_command));
        project->compile_commands->directory = strdup(dir_path);
        project->compile_commands->file_path = strdup(file_path);
        project->compile_commands->command = strdup(command);
        project->compile_commands->next = NULL;
      }

      compile_command *cc = project->compile_commands;

      while (cc->next != NULL) {
        cc = cc->next;
      }

      cc->next = malloc(sizeof(compile_command));
      cc->next->directory = strdup(dir_path);
      cc->next->file_path = strdup(file_path);
      cc->next->command = strdup(command);
      cc->next->next = NULL;

      free(dir_path);

    } else if (entry->d_type == DT_DIR) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      }
      char dir_path[MAX_PATH];
      snprintf(dir_path, MAX_PATH, "%s/%s", path, entry->d_name);

      if (!get_compile_commands(dir_path, path, project)) {
        fprintf(stderr, "Could not get files in %s\n", dir_path);
        closedir(dirp);
        return false;
      }
    }
  }

  closedir(dirp);
  return true;
}

static bool directory_exists(const char *path) {
  DIR *dir;
  if ((dir = opendir(path)) == NULL) {
    return false;
  }
  closedir(dir);
  return true;
}

static bool clone_dep(dependency *dependency) {
  char path[MAX_PATH];
  snprintf(path, MAX_PATH, "./target/deps/%s", dependency->name);
  if (!directory_exists(path)) {
    char command[MAX_COMMAND];
    snprintf(command, MAX_COMMAND, "git clone -q -- %s %s\n", dependency->url,
             path);

    printf("Downloading: %s\n", dependency->name);

    if (system(command) != 0) {
      fprintf(stderr, "Download failed: %s\n", command);
      return false;
    };
  }

  char include_path[MAX_PATH + 16];
  snprintf(include_path, MAX_PATH + 16, "%s/include", path);
  dependency->include_path = realpath(include_path, NULL);
  dependency->path = realpath(path, NULL);

  //  char kinetic_file_path[MAX_PATH];
  //  snprintf(kinetic_file_path, MAX_PATH, "%s/kinetic.kn", path);
  //
  //  char *kinetic_file = read_file_to_buffer(kinetic_file_path);
  //
  //  kn_definition *definition = create_definition();
  //  if (!kinetic_notation_parse(definition, kinetic_file)) {
  //    return false;
  //  }
  //
  //  struct get_object_array_length_result dependency_count_result =
  //      kn_definition_get_object_array_length(definition,
  //      DEPENDENCIES_KEY);
  //  if (dependency_count_result.result == NOT_FILLED_IN) {
  //  } else if (dependency_count_result.result != SUCCESS) {
  //    fprintf(stderr, "Could not get dependencies of %s\n",
  //    dependency->name); return false;
  //  }
  //
  //  for (size_t i = 0; i < dependency_count_result.length; ++i) {
  //    struct get_object_at_index_result object_result =
  //        kn_definition_get_object_from_array_at_index(definition,
  //                                                     DEPENDENCIES_KEY,
  //                                                     i);
  //    if (object_result.result != SUCCESS) {
  //      fprintf(stderr, "Could not get object at index: %zu\n", i);
  //      return false;
  //    }
  //  }

  return true;
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

  struct get_string_result flags_result =
      kn_definition_get_string(definition, FLAGS_KEY);
  if (flags_result.result == NOT_FILLED_IN) {
    project->compile_flags = "";
  } else if (flags_result.result != SUCCESS) {
    fprintf(stderr, "Could not get project flags\n");
    return false;
  } else {
    project->compile_flags = flags_result.string;
  }

  struct get_object_result lib_result =
      kn_definition_get_object(definition, LIBRARY_KEY);
  if (lib_result.result == NOT_FILLED_IN) {
    project->is_lib = false;
  } else if (lib_result.result != SUCCESS) {
    fprintf(stderr, "Could not get lib\n");
    return false;
  } else {
    project->is_lib = true;

    struct get_boolean_result lib_shared_result =
        kn_definition_get_boolean(lib_result.object, LIBRARY_SHARED_KEY);
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
  if (length_result.result == NOT_FILLED_IN) {
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
        if (get_dependency(dependency_result.object,
                           &project->dependencies[i]) == false) {
          fprintf(stderr, "Issue getting dependency\n");
          return false;
        }
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
      kn_definition_get_string(definition, DEPENDENCY_URL_KEY);
  if (url_result.result != SUCCESS) {
    return false;
  }
  (*dependency).url = url_result.string;

  return true;
}

static kn_definition *create_definition() {
  kn_definition *definition = kn_definition_new();

  kn_definition_add_string(definition, NAME_KEY);
  kn_definition_add_version(definition, VERSION_KEY);
  kn_definition_add_string(definition, FLAGS_KEY);

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
      mkdir(dir_name, 0777);
    } else {
      return false;
    }
  }

  closedir(dir);

  return true;
}
