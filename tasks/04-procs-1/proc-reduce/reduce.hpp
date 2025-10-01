#pragma once

#include <cstdint>
#include <numeric>
#include <ranges>
#include <unused.hpp>  // TODO: remove before flight.

template <class F>
uint64_t Reduce(uint64_t from, uint64_t to, uint64_t init, F&& f,
                size_t max_parallelism) {
    UNUSED(max_parallelism);  // TODO: remove before flight.
    auto r = std::views::iota(from, to);  // TODO: remove before flight.
    return std::reduce(r.begin(), r.end(), init, f);  // TODO: remove before flight.
}
