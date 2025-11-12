enable_language(ASM)

add_caos_library(task_syscalls STATIC syscalls.cpp)
target_compile_options(task_syscalls PRIVATE ${FREESTANDING_COMPILE_OPTIONS})

add_caos_executable(test_syscalls_nostd test-nostd.cpp)
target_link_libraries(test_syscalls_nostd PRIVATE task_syscalls)
target_compile_options(test_syscalls_nostd PRIVATE ${FREESTANDING_COMPILE_OPTIONS})
target_link_options(test_syscalls_nostd PRIVATE ${FREESTANDING_LINK_OPTIONS})

add_catch_executable(test_syscalls test.cpp)
target_link_libraries(test_syscalls PRIVATE task_syscalls caos_utils)
