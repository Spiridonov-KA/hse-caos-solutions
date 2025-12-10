enable_language(ASM)

file(GLOB_RECURSE SOURCES
    CONFIGURE_DEPENDS
    src/*.cpp src/*.S)

add_caos_executable(harness_backtrace_2 ${SOURCES} harness.cpp)
# set_property(
#     SOURCE harness.cpp
#     PROPERTY COMPILE_OPTIONS -O0
# )
target_compile_options(harness_backtrace_2 PRIVATE -fno-omit-frame-pointer)
target_link_libraries(harness_backtrace_2 PRIVATE caos_utils)
