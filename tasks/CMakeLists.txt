file(GLOB_RECURSE TASK_CMAKES
    CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "**/CMakeLists.txt")

foreach(task_cmake IN LISTS TASK_CMAKES)
    get_filename_component(TASK "${task_cmake}" DIRECTORY)
    message(STATUS "Found task ${TASK}")
    add_subdirectory(${TASK})
endforeach()
