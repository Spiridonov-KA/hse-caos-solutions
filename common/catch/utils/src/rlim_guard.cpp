#include <rlim_guard.hpp>

#include "internal_assert.hpp"

#include <sys/resource.h>
#include <utility>

static uint64_t ModifySoftLimit(int limit_type, uint64_t new_limit) {
    struct rlimit limit{};
    {
        int ret = getrlimit(limit_type, &limit);
        INTERNAL_ASSERT(ret != -1);
    }

    auto new_limit_inner = static_cast<rlim_t>(new_limit);
    INTERNAL_ASSERT(new_limit_inner <= limit.rlim_max);
    auto prev_limit = std::exchange(limit.rlim_cur, new_limit_inner);

    {
        int ret = setrlimit(limit_type, &limit);
        INTERNAL_ASSERT(ret != -1);
    }
    return static_cast<uint64_t>(prev_limit);
}

RLimGuard::RLimGuard(int limit_type, rlim_t new_limit)
    : limit_type_(limit_type) {
    prev_limit_ = ModifySoftLimit(limit_type_, new_limit);
}

void RLimGuard::Reset() {
    auto limit_type = std::exchange(limit_type_, -1);
    if (limit_type == -1) {
        return;
    }
    ModifySoftLimit(limit_type, prev_limit_);
}

RLimGuard::~RLimGuard() {
    Reset();
}
