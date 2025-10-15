enable_language(ASM)
add_catch_executable(test_arg_sum_2 test.cpp arg-sum-2.S)
target_link_libraries(test_arg_sum_2 PRIVATE caos_utils)
