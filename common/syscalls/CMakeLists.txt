file(GLOB_RECURSE CPP_SOURCES src/cpp/*.cpp)
file(GLOB_RECURSE ASM_SOURCES src/asm/${ARCHITECTURE}/*.S)

enable_language(ASM)

add_library(syscalls STATIC ${CPP_SOURCES} ${ASM_SOURCES})
target_include_directories(syscalls PUBLIC include)
target_include_directories(syscalls PRIVATE src)
