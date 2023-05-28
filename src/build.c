#include "compile.h"
#include "dependencies.h"
#include "directory.h"
#include "link.h"
#include "project_definition.h"

// std
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define TARGET_DIR "./target"
#define DEPENDENCY_CLONE_DIR "./target/deps"
#define BUILD_DIR "./target/build"
#define SOURCE_DIR "./src"

char *get_name() { return "build"; }

int execute(int argc, char *argv[]) {
	kinetic_project project = {0};
	if (!kinetic_project_from_dir(".", &project)) {
		fprintf(stderr, "failed to read project definition:\n");
		printf("%s\n", kinetic_notation_get_error());
		return 1;
	}

	if (!create_dir(TARGET_DIR)) {
		fprintf(stderr, "Failed to create directory: %s\n", TARGET_DIR);
		return 1;
	}
	project.target_dir = realpath(TARGET_DIR, NULL);

	printf("Cloning dependencies...\n");
	clone_dependencies_to(DEPENDENCY_CLONE_DIR, project);
	if (!create_dir(BUILD_DIR)) {
		fprintf(stderr, "Failed to create directory: %s\n", BUILD_DIR);
		return 1;
	}
	project.build_dir = realpath(BUILD_DIR, NULL);

	printf("Getting compile commands...\n");
	char project_build_dir[sizeof(BUILD_DIR) + sizeof(project.name) + 1];
	sprintf(project_build_dir, "%s/%s", BUILD_DIR, project.name);
	compile_commands project_compile_commands = {};
	if (!get_compile_commands_for_directory(SOURCE_DIR,
											project_build_dir,
											false,
											"",
											project,
											&project_compile_commands)) {
		fprintf(stderr, "Failed to get compile commands for project!\n");
		return 1;
	}

	generate_compile_commands_json(project_compile_commands);

	if (project.dependency_count > 0) {
		printf("Compiling dependencies...\n");
		if (!compile_dependencies(DEPENDENCY_CLONE_DIR, BUILD_DIR)) {
			fprintf(stderr, "Failed to compile dependencies\n");
			return 1;
		}
	}

	printf("Compiling project...\n");
	run_commands(project_compile_commands);

	if (!link_objects(project)) { fprintf(stderr, "Failed to link\n"); }

	kinetic_project_clear(&project);
	return 0;
}
