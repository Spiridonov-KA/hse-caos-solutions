include(CheckCXXCompilerFlag)

set(CMAKE_CXX_FLAGS_ASAN
    "${CMAKE_CXX_FLAGS_ASAN} -g -fsanitize=address,undefined -fno-sanitize-recover=all -DASAN"
    CACHE STRING "Additional compiler flags in asan builds"
    FORCE
)

set(CMAKE_CXX_FLAGS_TSAN
    "${CMAKE_CXX_FLAGS_TSAN} -g -fsanitize=thread -fno-sanitize-recover=all -DTSAN"
    CACHE STRING "Additional compiler flags in asan builds"
    FORCE
)

set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -g -DDEBUG"
    CACHE STRING "Additional compiler flags in debug builds"
    FORCE
)

set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} -O2 -g -DRELEASE -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3"
    CACHE STRING "Additional compiler flags in debug builds"
    FORCE
)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror"
)

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
