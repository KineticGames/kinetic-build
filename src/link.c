#include "link.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_COMMAND 1024

bool link_objects(kinetic_project project) {
	char link_command[MAX_COMMAND];
	if (project.is_lib) {
		if (project.lib.shared) {
			snprintf(link_command,
					 MAX_COMMAND,
					 "/usr/bin/cc -shared -o %s/target/lib%s.so %s/target/build/*.o",
					 project.project_dir,
					 project.name,
					 project.project_dir);
		} else {
			snprintf(link_command,
					 MAX_COMMAND,
					 "/usr/bin/ar rcs %s/target/%s %s/target/build/*.o",
					 project.project_dir,
					 project.name,
					 project.project_dir);
		}
	} else {
		snprintf(link_command,
				 MAX_COMMAND,
				 "/usr/bin/cc -std=gnu11 -o %s/target/lib%s.a %s/target/build/*.o",
				 project.project_dir,
				 project.name,
				 project.project_dir);
	}

	if (system(link_command) != 0) { return false; }

	return true;
}
