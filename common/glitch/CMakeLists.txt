if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "glitch is Linux-only (relies on GNU ld's --wrap).")
endif()

file(GLOB_RECURSE SOURCES
    CONFIGURE_DEPENDS
    src/*.cpp)

add_library(glitch STATIC ${SOURCES})
target_include_directories(glitch PUBLIC include)

# Pinnacle of overengineering
set(_emit "${CMAKE_CURRENT_BINARY_DIR}/emit-glitch-syms.cpp")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/emit-glitch-syms.cpp.in" "${_emit}" @ONLY)

try_run(GLITCH_EMIT_STATUS GLITCH_EMIT_COMPILE_STATUS
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${_emit}"
    RUN_OUTPUT_VARIABLE GLITCH_EMIT_OUT
    COMPILE_OUTPUT_VARIABLE GLITCH_EMIT_COMPILE_OUT
)

if (NOT GLITCH_EMIT_COMPILE_STATUS OR NOT GLITCH_EMIT_STATUS EQUAL 0)
    message(FATAL_ERROR
        "Failed to list glitch wrapped symbols."
        "Compile out:\n${GLITCH_EMIT_COMPILE_OUT}\n"
        "Run out:\n${GLITCH_EMIT_OUT}\n"
        "Statuses: ${GLITCH_EMIT_COMPILE_STATUS} ${GLITCH_EMIT_STATUS}"
    )
endif()

set(GLITCH_WRAP_SYMS "${GLITCH_EMIT_OUT}")
list(REMOVE_ITEM GLITCH_WRAP_SYMS "")

foreach(sym IN LISTS GLITCH_WRAP_SYMS)
    target_link_options(glitch INTERFACE "-Wl,-wrap=${sym}")
endforeach()
