enable_language(ASM)
add_catch_executable(test_tree_sum test.cpp tree-sum.S)
target_link_libraries(test_tree_sum PRIVATE caos_utils)
