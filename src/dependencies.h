#ifndef KINETIC_BUILD_DEPENDENCIES_H
#define KINETIC_BUILD_DEPENDENCIES_H

#include "directory.h"
#include "project_definition.h"

// std
#include <stdbool.h>

bool clone_dependencies_to(const char *path, kinetic_project project);

#endif // KINETIC_BUILD_DEPENDENCIES_H
