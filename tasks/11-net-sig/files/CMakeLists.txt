add_caos_executable(solution_files solution.cpp test_utils/wrappers.c)
target_link_options(solution_files PRIVATE
    -Wl,--wrap=sigprocmask
    -Wl,--wrap=sigsuspend
    -Wl,--wrap=pause
    -Wl,--wrap=main
)

add_caos_executable(initializer_files test_utils/initializer.c)
add_caos_executable(interactor_files test_utils/interactor.c)
