enable_language(ASM)

file(GLOB_RECURSE SOURCES
    CONFIGURE_DEPENDS
    src/*.cpp src/*.S)

add_caos_library(coro STATIC ${SOURCES})
target_include_directories(coro PUBLIC src)

add_catch_executable(test_coro tests.cpp)
target_link_libraries(test_coro PRIVATE caos_utils coro benchmark)

file(GLOB_RECURSE PLAYGROUND_SOURCES
    CONFIGURE_DEPENDS
    playground/*.cpp)
add_caos_executable(play_coro ${PLAYGROUND_SOURCES})
target_link_libraries(play_coro PRIVATE coro)
