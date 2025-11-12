enable_language(ASM)
file(GLOB_RECURSE SOURCES
    CONFIGURE_DEPENDS
    src/*.cpp src/*.S)

add_caos_executable(solution_print_args ${SOURCES})
target_compile_options(solution_print_args PRIVATE ${FREESTANDING_COMPILE_OPTIONS})
target_link_options(solution_print_args PRIVATE ${FREESTANDING_LINK_OPTIONS})
