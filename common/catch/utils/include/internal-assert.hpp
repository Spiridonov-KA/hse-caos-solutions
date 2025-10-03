#include <catch2/catch_test_macros.hpp>

#define INTERNAL_ASSERT(cond)                                                  \
    do {                                                                       \
        INFO("This is an internal assert. Report if it fails");                \
        REQUIRE(cond);                                                         \
    } while (false)
