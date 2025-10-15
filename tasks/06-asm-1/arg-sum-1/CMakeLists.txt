enable_language(ASM)
add_catch_executable(test_arg_sum_1 test.cpp arg-sum-1.S)
target_link_libraries(test_arg_sum_1 PRIVATE caos_utils)
