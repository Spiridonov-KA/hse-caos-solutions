file(GLOB_RECURSE CPP_SOURCES
    CONFIGURE_DEPENDS
    src/*.cpp)

add_library(benchmark STATIC ${CPP_SOURCES})
target_include_directories(benchmark PUBLIC include)
target_include_directories(benchmark PRIVATE src)
