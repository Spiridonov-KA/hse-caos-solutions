cmake_minimum_required(VERSION 3.14)
project(hse-caos)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

include(cmake/Arch.cmake)
include(cmake/Helpers.cmake)
include(cmake/Flags.cmake)

add_subdirectory(common)
include_directories(common/include)

add_subdirectory(tasks)
