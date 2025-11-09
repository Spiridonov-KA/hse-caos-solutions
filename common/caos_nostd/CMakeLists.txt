file(GLOB_RECURSE CPP_SOURCES
    CONFIGURE_DEPENDS
    src/*.cpp)
file(GLOB_RECURSE ASM_SOURCES
    CONFIGURE_DEPENDS
    src/${ARCHITECTURE}/*.S)

enable_language(ASM)

set_property(
    SOURCE builtins.cpp
    PROPERTY
        COMPILE_OPTIONS
            -fno-builtin-memset -fno-builtin-memcpy -fno-builtin-memmove
            -fno-builtin-memcmp -fno-tree-loop-distribute-patterns
)

add_library(caos_nostd_flags INTERFACE)
# TODO: Maybe add -fno-rtti
target_compile_options(caos_nostd_flags INTERFACE ${FREESTANDING_COMPILE_OPTIONS})

add_library(caos_nostd STATIC ${CPP_SOURCES} ${ASM_SOURCES})
target_link_libraries(caos_nostd PUBLIC caos_nostd_flags syscalls)
target_link_options(caos_nostd PUBLIC ${FREESTANDING_LINK_OPTIONS})
target_include_directories(caos_nostd PUBLIC include)
