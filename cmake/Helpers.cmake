set(ASM_FLAGS "")

if (ARCHITECTURE STREQUAL "x86_64")
    SET(ASM_FLAGS "-masm=intel")
endif()

function (add_caos_executable name)
    add_executable(${name} ${ARGN})
    target_compile_options(${name} PRIVATE "${ASM_FLAGS}")
endfunction()

function (add_caos_library name)
    add_library(${name} ${ARGN})
    target_compile_options(${name} PRIVATE "${ASM_FLAGS}")
endfunction()
