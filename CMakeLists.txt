cmake_minimum_required(VERSION 3.22)

project(kinetic-build)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-Wall -ggdb")

add_library(build SHARED src/build.c)

add_subdirectory(lib)

target_link_libraries(build PRIVATE kinetic_notation)
