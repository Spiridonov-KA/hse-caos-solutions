add_catch_executable(test_limbo test.cpp)
target_link_libraries(test_limbo PRIVATE caos_utils benchmark)
