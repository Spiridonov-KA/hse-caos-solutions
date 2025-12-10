add_caos_executable(solution_ignore cat.cpp test_utils/wrappers.c)
target_link_options(solution_ignore PRIVATE
    -Wl,--wrap=main
    -Wl,--wrap=sigaction
    -Wl,--wrap=signal
    -Wl,--wrap=dprintf
    -Wl,--wrap=fprintf
    -Wl,--wrap=fputc
    -Wl,--wrap=fputs
    -Wl,--wrap=fwrite
    -Wl,--wrap=printf
    -Wl,--wrap=putc
    -Wl,--wrap=putchar
    -Wl,--wrap=puts
    -Wl,--wrap=fflush
    -Wl,--wrap=write
)

add_caos_executable(run_interactor_ignore test_utils/run_interactor.c)
add_caos_executable(interactor_ignore test_utils/interactor.c)
