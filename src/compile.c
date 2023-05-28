#include "compile.h"

#include "directory.h"
#include "file_ops.h"

// std
#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_PATH
	#define MAX_PATH 1024
#endif // MAX_PATH

#ifndef MAX_INCLUDE
	#define MAX_INCLUDE 6144
#endif // MAX_INCLUDE

#ifndef MAX_COMMAND
	#define MAX_COMMAND 8192
#endif // MAX_COMMAND

#ifndef COMPILE_FLAGS
	#define COMPILE_FLAGS "-g -Wall -Wextra"
#endif // COMPILE_FLAGS

struct command {
	char *directory;
	char *command;
	char *file;
	struct command *next;
};

bool get_compile_commands_for_directory(const char *source_path,
										const char *target_path,
										bool is_subdir,
										const char *object_file_prefix,
										kinetic_project project,
										compile_commands *out_commands) {
	char *full_source_path = realpath(source_path, NULL);

	if (!create_dir(target_path)) {
		fprintf(stderr, "Failed to create: %s", target_path);
		return false;
	}
	char *full_target_path = realpath(target_path, NULL);

	if (!directory_exists(full_source_path)) {
		fprintf(stderr, "Could not find directory: '%s'", source_path);
		return false;
	}

	DIR *dir;
	if ((dir = opendir(full_source_path)) == NULL) {
		fprintf(stderr, "Could not open directory: '%s'", source_path);
		return false;
	}

	char includes[MAX_INCLUDE] = "";
	char project_include_dir[strlen(project.project_dir) + strlen("/include")];
	sprintf(project_include_dir, "%s/include", project.project_dir);
	if (directory_exists(project_include_dir)) { snprintf(includes, MAX_INCLUDE, "-I%s", project_include_dir); }

	for (size_t i = 0; i < project.dependency_count; ++i) {
		char new_include[MAX_PATH];
		snprintf(new_include,
				 MAX_PATH,
				 " -I%s/deps/%s-%zu_%zu_%zu/include",
				 project.target_dir,
				 project.dependencies[i].name,
				 project.dependencies[i].version.major,
				 project.dependencies[i].version.minor,
				 project.dependencies[i].version.patch);
		size_t x = MAX_INCLUDE - strlen(includes) - 1;
		strncat(includes, new_include, x);
	}

	size_t compile_flags_length = strlen(COMPILE_FLAGS) + 7;
	char compile_flags[compile_flags_length];
	strcpy(compile_flags, COMPILE_FLAGS);
	if (project.is_lib && project.lib.shared) { strcat(compile_flags, " -fPIC "); }

	struct command *first_command        = NULL;
	struct command *command              = NULL;
	size_t command_count                 = 0;
	compile_commands all_subdir_commands = {};

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		switch (entry->d_type) {
			case DT_REG: {
				const char *dot = strrchr(entry->d_name, '.');
				if (strcmp(dot, ".c") != 0) { continue; }

				char source_file[strlen(full_source_path) + strlen(entry->d_name) + 1];
				sprintf(source_file, "%s/%s", full_source_path, entry->d_name);

				size_t name_len = (void *)dot - (void *)&entry->d_name + 1;
				if (is_subdir) { name_len += strlen(object_file_prefix); }
				char file_name_woe[name_len];
				snprintf(file_name_woe, name_len, "%s%s", is_subdir ? object_file_prefix : "", entry->d_name);

				if (first_command == NULL) {
					first_command = calloc(1, sizeof(struct command));
					command       = first_command;
				} else {
					command->next = calloc(1, sizeof(struct command));
					command       = command->next;
				}

				command->command = calloc(MAX_COMMAND, sizeof(char));
				snprintf(command->command,
						 MAX_COMMAND,
						 "/usr/bin/cc %s -I%s/src %s -std=gnu11 -o %s/%s.o -c %s",
						 includes,
						 project.project_dir,
						 compile_flags,
						 full_target_path,
						 file_name_woe,
						 source_file);

				command->file      = strdup(source_file);
				command->directory = strdup(source_path);

				command_count++;
			} break;
			case DT_DIR: {
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { continue; }

				char subdir_path[MAX_PATH];
				snprintf(subdir_path, MAX_PATH, "%s/%s", source_path, entry->d_name);

				size_t subdir_prefix_length = strlen(entry->d_name) + 1;

				if (is_subdir) { subdir_prefix_length += strlen(object_file_prefix); }

				char subdir_prefix[subdir_prefix_length];

				snprintf(subdir_prefix,
						 subdir_prefix_length,
						 "%s%s_",
						 is_subdir ? object_file_prefix : "",
						 entry->d_name);

				compile_commands subdir_commands;
				if (!get_compile_commands_for_directory(subdir_path,
														target_path,
														true,
														subdir_prefix,
														project,
														&subdir_commands)) {
					fprintf(stderr, "Failed to get compile all_subdir_commands for files in: %s", subdir_path);
					return false;
				}

				combine_compile_commands(&all_subdir_commands, subdir_commands);
			} break;
			default:
				break;
		}
	}

	command                     = first_command;
	out_commands->command_count = command_count;
	out_commands->commands      = calloc(command_count, sizeof(compile_command));
	for (size_t i = 0; i < command_count; ++i) {
		out_commands->commands[i].command = command->command;

		struct command *next = command->next;
		free(command);
		command = next;
	}

	combine_compile_commands(out_commands, all_subdir_commands);

	free(full_source_path);
	free(full_target_path);

	closedir(dir);
	return true;
}

void combine_compile_commands(compile_commands *dest, compile_commands source) {
	size_t new_count = dest->command_count + source.command_count;

	dest->commands = reallocarray(dest->commands, new_count, sizeof(compile_command));
	memcpy(&dest->commands[dest->command_count], source.commands, source.command_count * sizeof(compile_command));

	dest->command_count = new_count;
}

bool compile_dependencies(const char *clone_dir, const char *build_dir) {
	DIR *dir;
	if ((dir = opendir(clone_dir)) == NULL) {
		fprintf(stderr, "Could not open directory: '%s'\n", clone_dir);
		return false;
	}

	bool success = true;

	char *full_clone_dir = realpath(clone_dir, NULL);
	char *full_build_dir = realpath(build_dir, NULL);

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
		printf("Compiling dependency: %s\n", entry->d_name);

		char dependency_clone_dir[sizeof(full_clone_dir) + sizeof(entry->d_name) + 1];
		sprintf(dependency_clone_dir, "%s/%s", full_clone_dir, entry->d_name);

		char dependency_build_dir[sizeof(full_build_dir) + sizeof(entry->d_name) + 1 + 60];
		sprintf(dependency_build_dir, "%s/%s", full_build_dir, entry->d_name);

		kinetic_project dependency_project = {0};
		if (!kinetic_project_from_dir(dependency_clone_dir, &dependency_project)) {
			fprintf(stderr, "Failed to parse dependency project at: %s\n", dependency_clone_dir);
			return false;
		}

		char dependency_source_dir[sizeof(dependency_clone_dir) + 4];
		sprintf(dependency_source_dir, "%s/src", dependency_clone_dir);

		compile_commands dependency_commands = {};
		if (!get_compile_commands_for_directory(dependency_source_dir,
												dependency_build_dir,
												false,
												"",
												dependency_project,
												&dependency_commands)) {
			fprintf(stderr, "Failed to get compile commands for dependency: %s\n", dependency_project.name);
		}

		success &= run_commands(dependency_commands);
	}

	free(full_clone_dir);
	free(full_build_dir);

	return success;
}

bool generate_compile_commands_json(compile_commands commands) {
	file_ops("compile_commands.json", "w") {
		if (fp == NULL) {
			fclose(fp);
			return false;
		}

		fprintf(fp, "[");

		for (size_t i = 0; i < commands.command_count; ++i) {
			fprintf(fp,
					"\n{\n\t\"directory\": \"%s\",\n\t\"command\": \"%s\",\n\t\"file\":"
					"\"%s\"\n}",
					commands.commands[i].directory,
					commands.commands[i].command,
					commands.commands[i].file);
			if (i + 1 < commands.command_count) { fprintf(fp, ","); }
		}
		fprintf(fp, "\n]");
	}

	return true;
}

bool run_commands(compile_commands commands) {
	for (size_t i = 0; i < commands.command_count; ++i) {
		if (system(commands.commands[i].command) != 0) {
			fprintf(stderr, "Failed while running: %s\n", commands.commands[i].command);
			return false;
		}
	}
	return true;
}
