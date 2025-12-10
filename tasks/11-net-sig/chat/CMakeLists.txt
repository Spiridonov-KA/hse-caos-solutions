add_caos_executable(solution_chat solution.cpp test_utils/wrappers.c)
target_link_options(solution_chat PRIVATE
    -Wl,--wrap=bind
    -Wl,--wrap=listen
)
