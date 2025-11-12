enable_language(ASM)

add_catch_executable(test_indirect_call test.cpp wrappers.S solution.S)
