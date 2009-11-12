project(pgl-gui)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

add_subdirectory(src)
#Create the executable in the working dir
SET(COMMON_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
SET_TARGET_PROPERTIES(pgl-gui PROPERTIES RUNTIME_OUTPUT_DIRECTORY
"${COMMON_RUNTIME_OUTPUT_DIRECTORY}")

