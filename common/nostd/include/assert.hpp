#pragma once

#include <io.hpp>
#include <syscalls.hpp>

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

// Variation of assert that makes debugging much more fun
#define ASSERT_NO_REPORT(cond)                                                 \
    do {                                                                       \
        if (!(cond)) {                                                         \
            ::Exit(2);                                                         \
        }                                                                      \
    } while (false)

#define ASSERT_M(cond, message)                                                \
    do {                                                                       \
        if (!(cond)) {                                                         \
            ::Print("Line " STRINGIFY(__LINE__) " Condition " #cond            \
                                                " failed: " message "\n");     \
            ::Exit(1);                                                         \
        }                                                                      \
    } while (false)

#define ASSERT_NO_M(cond)                                                      \
    do {                                                                       \
        if (!(cond)) {                                                         \
            ::Print("Line " STRINGIFY(__LINE__) " Condition " #cond "\n");     \
            ::Exit(1);                                                         \
        }                                                                      \
    } while (false)

#define _ASSERT_DISPATCH(a, b, c, ...) c

#define ASSERT(...)                                                            \
    _ASSERT_DISPATCH(__VA_ARGS__, ASSERT_M, ASSERT_NO_M)(__VA_ARGS__)

#define RUN_TEST(test, ...)                                                    \
    do {                                                                       \
        ::Print("Running " #test "\n");                                        \
        test(__VA_ARGS__);                                                     \
    } while (false)
