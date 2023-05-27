#include "compile.h"
#include "dependencies.h"
#include "directory.h"
#include "link.h"
#include "project_definition.h"

// std
#include <stdio.h>

#define TARGET_DIR "./target"
#define DEPENDENCY_CLONE_DIR "./target/deps"
#define BUILD_DIR "./target/build"

char *get_name() { return "build"; }

int execute(int argc, char *argv[]) {
	kinetic_project project = {0};
	if (!kinetic_project_from_dir(".", &project)) { return 1; }

	create_dir(TARGET_DIR);

	clone_dependencies_to(DEPENDENCY_CLONE_DIR, project);
	create_dir(BUILD_DIR);

	char project_build_dir[sizeof(BUILD_DIR) + sizeof(project.name) + 1];
	sprintf(project_build_dir, "%s/%s", BUILD_DIR, project.name);
	compile_commands project_compile_commands = get_compile_commands_for_directory("./src", project_build_dir, project);

	create_dir("target/build");

	compile_dependencies(DEPENDENCY_CLONE_DIR, BUILD_DIR);
	run_commands(project_compile_commands);

	generate_compile_commands_json(project_compile_commands);

	if (!link_objects(project)) { fprintf(stderr, "Failed to link\n"); }

	kinetic_project_clear(&project);
	return 0;
}
