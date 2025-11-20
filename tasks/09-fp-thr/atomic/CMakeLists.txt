enable_language(ASM)
add_catch_executable(test_atomic test.cpp atomic.S)
target_link_libraries(test_atomic PRIVATE caos_utils)
