enable_language(ASM)
add_catch_executable(test_args_fwd test.cpp call.S args-fwd.S)
target_link_libraries(test_args_fwd PRIVATE caos_utils)
