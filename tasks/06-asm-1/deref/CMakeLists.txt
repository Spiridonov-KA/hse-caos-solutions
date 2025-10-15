enable_language(ASM)
add_catch_executable(test_deref test.cpp deref.S)
target_link_libraries(test_deref PRIVATE caos_utils)
