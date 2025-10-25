enable_language(ASM)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.S)

add_caos_library(long_jump STATIC ${SOURCES})
target_include_directories(long_jump PUBLIC src)

add_caos_executable(test_long_jump_nostd test-nostd.cpp)
target_link_libraries(test_long_jump_nostd PRIVATE caos_nostd caos_utils long_jump)

add_catch_executable(test_long_jump test.cpp)
target_link_libraries(test_long_jump PRIVATE long_jump caos_utils)

file(GLOB_RECURSE PLAYGROUND_SRC CONFIGURE_DEPENDS playground/*.cpp)
add_caos_executable(play_long_jump ${PLAYGROUND_SRC})
target_link_libraries(play_long_jump PRIVATE long_jump)
