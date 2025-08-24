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
    "${CMAKE_CXX_FLAGS_RELEASE} -O2 -g -DRELEASE"
    CACHE STRING "Additional compiler flags in debug builds"
    FORCE
)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror"
)
