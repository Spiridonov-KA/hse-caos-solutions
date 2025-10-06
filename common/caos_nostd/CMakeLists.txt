file(GLOB_RECURSE CPP_SOURCES
    CONFIGURE_DEPENDS
    src/*.cpp)
file(GLOB_RECURSE ASM_SOURCES
    CONFIGURE_DEPENDS
    src/${ARCHITECTURE}/*.S)

enable_language(ASM)

add_library(caos_nostd STATIC ${CPP_SOURCES} ${ASM_SOURCES})
# TODO: Maybe add -fno-rtti
target_compile_options(caos_nostd PUBLIC -nostdlib -ffreestanding -fno-stack-protector -fno-exceptions)
target_link_options(caos_nostd PUBLIC -nostdlib -static)
target_link_libraries(caos_nostd PUBLIC syscalls)
target_include_directories(caos_nostd PUBLIC include)
