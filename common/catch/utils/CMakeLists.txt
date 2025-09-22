Include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)

FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_library(catch_utils STATIC ${SOURCES})
target_include_directories(catch_utils PUBLIC include)
target_include_directories(catch_utils PRIVATE src)
target_link_libraries(catch_utils PUBLIC Catch2::Catch2)
