file(GLOB_RECURSE CPP_SOURCES
    CONFIGURE_DEPENDS
    src/*.cpp)
file(GLOB_RECURSE ASM_SOURCES
    CONFIGURE_DEPENDS
    src/${ARCHITECTURE}/*.S)

enable_language(ASM)

add_library(syscalls STATIC ${CPP_SOURCES} ${ASM_SOURCES})
target_include_directories(syscalls PUBLIC include)
