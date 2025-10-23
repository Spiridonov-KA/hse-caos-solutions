include(CheckCXXCompilerFlag)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    # NOTE: FORTIFY_SOURCE may cause issues in freestanding builds
    add_compile_options(-UNDEBUG -O2 -DRELEASE -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_compile_options(-DDEBUG)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Asan")
    add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all -DASAN)
    add_link_options(-fsanitize=address,undefined)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Tsan")
    add_compile_options(-fsanitize=thread -fno-sanitize-recover=all -DTSAN)
    add_link_options(-fsanitize=thread)
endif()

add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic;-Werror;-g>")

function (add_flag_if_supported flag)
    string(MAKE_C_IDENTIFIER "${flag}" _id)
    set(_has "HAS_${_id}")

    check_cxx_compiler_flag("${flag}" ${_has})
    if (${_has})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
    endif()
endfunction()

add_flag_if_supported("-Wno-array-bounds")
add_flag_if_supported("-Wno-stringop-overread")
