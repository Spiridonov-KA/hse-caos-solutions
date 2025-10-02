file(GLOB_RECURSE SOURCES src/*.cpp)

add_library(caos_utils STATIC ${SOURCES})
target_include_directories(caos_utils PUBLIC include)
