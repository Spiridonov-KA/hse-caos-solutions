add_caos_executable(test_allocator allocator.cpp test.cpp)
target_link_libraries(test_allocator PRIVATE caos_nostd)
