#include "link.h"

// std
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND 3072
#define MAX_BUILD_TARGET_LENGTH 2048

bool link_objects(kinetic_project project) {
	char object_files[MAX_BUILD_TARGET_LENGTH] = "";

	DIR *dir;
	if ((dir = opendir(project.build_dir)) == NULL) {
		fprintf(stderr, "Could not open directory: '%s'", project.build_dir);
		return false;
	}
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
		size_t object_files_length = strlen(object_files);
		snprintf(object_files + object_files_length,
				 MAX_BUILD_TARGET_LENGTH - object_files_length,
				 " %s/%s/*.o",
				 project.build_dir,
				 entry->d_name);
	}

	char link_command[MAX_COMMAND];
	if (project.is_lib) {
		if (project.lib.shared) {
			printf("linking shared library\n");
			snprintf(link_command,
					 MAX_COMMAND,
					 "/usr/bin/cc -shared -o %s/target/lib%s.so %s",
					 project.project_dir,
					 project.name,
					 object_files);
		} else {
			printf("linking static library\n");
			snprintf(link_command,
					 MAX_COMMAND,
					 "/usr/bin/ar -rcs %s/target/lib%s.a %s",
					 project.project_dir,
					 project.name,
					 object_files);
		}
	} else {
		printf("linking executable\n");
		snprintf(link_command,
				 MAX_COMMAND,
				 "/usr/bin/cc -o %s/target/%s %s",
				 project.project_dir,
				 project.name,
				 object_files);
	}

	if (system(link_command) != 0) {
		fprintf(stderr, "Failed running: %s\n", link_command);
		return false;
	}

	return true;
}
