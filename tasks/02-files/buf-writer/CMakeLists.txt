add_catch_executable(test_buf_writer test.cpp wrappers.cpp)
target_link_options(test_buf_writer PRIVATE -Wl,-wrap=write)
