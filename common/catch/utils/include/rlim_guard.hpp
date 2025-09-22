#pragma once

#include <cstdint>

struct RLimGuard {
    RLimGuard(int limit_type, uint64_t new_limit);

    RLimGuard(const RLimGuard&) = delete;
    RLimGuard(RLimGuard&&) = delete;

    RLimGuard& operator=(const RLimGuard&) = delete;
    RLimGuard& operator=(RLimGuard&&) = delete;

    void Reset();

    ~RLimGuard();

  private:
    int limit_type_;
    uint64_t prev_limit_;
};
