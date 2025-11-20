add_catch_executable(test_queue test.cpp)
target_link_libraries(test_queue PRIVATE caos_utils benchmark)
