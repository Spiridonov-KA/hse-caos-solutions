Include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)

FetchContent_MakeAvailable(Catch2)

add_library(ut_main ut_main.cpp)
target_link_libraries(ut_main Catch2::Catch2WithMain)

function(add_catch_executable name)
    add_caos_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE ut_main)
endfunction()
