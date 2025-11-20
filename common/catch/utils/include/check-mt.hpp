#pragma once

#include <mutex>

namespace detail {
inline std::mutex check_m;
}

#define CHECK_MT(cond)                                                         \
    do {                                                                       \
        if (!(cond)) [[unlikely]] {                                            \
            std::lock_guard lk{::detail::check_m};                             \
            CHECK(cond);                                                       \
        }                                                                      \
    } while (false)
