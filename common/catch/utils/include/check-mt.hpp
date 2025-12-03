#pragma once

#include <mutex>

namespace detail {
inline std::mutex check_m;
}

#define _WITH_LOCK(expr, macro)                                                \
    do {                                                                       \
        if (!(expr)) [[unlikely]] {                                            \
            std::lock_guard lk{::detail::check_m};                             \
            macro(expr);                                                       \
        }                                                                      \
    } while (false)

#define CHECK_MT(cond) _WITH_LOCK(cond, CHECK)
#define REQUIRE_MT(cond) _WITH_LOCK(cond, REQUIRE)
