set_property(
    SOURCE mul8.cpp
    PROPERTY
        COMPILE_OPTIONS -mno-80387 -mno-sse -mno-sse2 -mno-avx -mno-fma -msoft-float
)
add_caos_executable(solution_mul8 main.cpp mul8.cpp)
