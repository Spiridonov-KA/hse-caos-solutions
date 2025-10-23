enable_language(ASM)
add_caos_executable(solution_backtrace main.cpp solution.S)
target_compile_options(solution_backtrace PRIVATE -fno-omit-frame-pointer -g)
