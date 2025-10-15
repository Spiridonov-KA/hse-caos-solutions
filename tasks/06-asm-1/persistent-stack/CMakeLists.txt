enable_language(ASM)
add_catch_executable(test_persistent_stack test.cpp stack.S)
target_link_libraries(test_persistent_stack PRIVATE caos_utils)
