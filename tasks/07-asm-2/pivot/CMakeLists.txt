enable_language(ASM)
add_catch_executable(test_pivot test.cpp solution.S call.S)
target_link_libraries(test_pivot PRIVATE caos_utils)
