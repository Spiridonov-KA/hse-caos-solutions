add_catch_executable(test_interval test.cpp)
target_link_libraries(test_interval PRIVATE caos_utils)
# https://clang.llvm.org/docs/UsersManual.html#cmdoption-f-no-rounding-math
target_compile_options(test_interval PRIVATE -frounding-math)
