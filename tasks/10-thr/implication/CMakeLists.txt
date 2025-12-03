enable_language(ASM)
add_catch_executable(test_implication test.cpp implication.cpp)
target_link_libraries(test_implication PRIVATE caos_utils benchmark)
