#include "reduce.hpp"

#include <benchmark/run.hpp>
#include <build.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <numeric>
#include <ranges>

template <class F>
uint64_t CorrectReduce(uint64_t from, uint64_t to, uint64_t init, F&& f) {
    auto r = std::views::iota(from, to);
    return std::reduce(r.begin(), r.end(), init, std::forward<F>(f));
}

#define CHECK_SAME_RESULT(from, to, init, f, par)                              \
    CHECK(Reduce(from, to, init, f, par) == CorrectReduce(from, to, init, f));

uint64_t OrMul(uint64_t lhs, uint64_t rhs) {
    return (lhs | 1) * (rhs | 1);
}

TEST_CASE("Correctness") {
    CHECK_SAME_RESULT(1, 1, 1, std::plus{}, 1);
    CHECK_SAME_RESULT(1, 1, 1, std::plus{}, 1);

    CHECK_SAME_RESULT(0, 1000, 1, OrMul, 4);
    CHECK_SAME_RESULT(0, 10'000, 1, OrMul, 4);
    CHECK_SAME_RESULT(0, 10'000'000, 1, OrMul, 4);
    CHECK_SAME_RESULT(0, 100'000'000, 1, OrMul, 4);
    CHECK_SAME_RESULT(1000, 10'000'000, 123, OrMul, 4);
}

TEST_CASE("Performance") {
    if constexpr (kBuildType != BuildType::Release) {
        return;
    }

    uint64_t twist = Catch::getSeed();
    auto op = [twist](uint64_t lhs, uint64_t rhs) {
        lhs = (lhs ^ twist) | 1;
        rhs = (rhs ^ twist) | 1;
        return (lhs * rhs) ^ twist;
    };

    uint64_t from = 1000;
    uint64_t to = 1'000'000'000;
    auto par_reduce = RunWithWarmup(
        [op, from, to] { return Reduce(from, to, 1, op, 4); }, 1, 5);

    auto seq_reduce = RunWithWarmup(
        [op, from, to] { return CorrectReduce(from, to, 1, op); }, 1, 5);

    CHECK_SAME_RESULT(from, to, 1, op, 4);

    auto ratio = double(seq_reduce.wall_time.count()) /
                 double(par_reduce.wall_time.count());

    WARN("Parallel version is " << std::fixed << std::setprecision(3) << ratio
                                << " times faster than the ordinary one");
    CHECK(ratio > 2);
}
