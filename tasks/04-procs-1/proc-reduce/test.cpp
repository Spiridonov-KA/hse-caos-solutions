#include "reduce.hpp"

#include <benchmark/run.hpp>
#include <build.hpp>
#include <fd_guard.hpp>
#include <pcg_random.hpp>

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

uint64_t Inv64(uint64_t x) {
    uint64_t inv = 1;
    for (size_t i = 0; i < 62; ++i) {
        inv *= x;
        x *= x;
    }
    return inv;
}

template <class F>
auto Conjugate(F op, uint64_t seed) {
    PCGRandom rng{seed};
    rng.Warmup();

    auto k = rng.Generate64() | 1;
    auto b = rng.Generate64();
    auto invk = Inv64(k);

    return [op = std::move(op), k, b, invk](uint64_t lhs, uint64_t rhs) {
        lhs = k * lhs + b;
        rhs = k * rhs + b;

        auto result = op(lhs, rhs);
        return (result - b) * invk;
    };
}

#define CHECK_SAME_RESULT(from, to, init, f, par)                              \
    do {                                                                       \
        auto correct = Reduce(from, to, init, f, par);                         \
        auto actual = CorrectReduce(from, to, init, f);                        \
        CHECK(correct == actual);                                              \
    } while (false)

uint64_t OrMul(uint64_t lhs, uint64_t rhs) {
    return (lhs | 1) * (rhs | 1);
}

TEST_CASE("Correctness") {
    FileDescriptorsGuard guard;

    CHECK_SAME_RESULT(1, 1, 1, std::plus{}, 1);
    CHECK_SAME_RESULT(1, 1, 1, std::plus{}, 20);

    CHECK_SAME_RESULT(0, 1000, 1, OrMul, 4);
    CHECK_SAME_RESULT(0, 10'000, 1, OrMul, 4);
    CHECK_SAME_RESULT(0, 10'000'000, 1, OrMul, 4);
    CHECK_SAME_RESULT(0, 100'000'000, 1, OrMul, 4);
    CHECK_SAME_RESULT(1000, 10'000'000, 123, OrMul, 4);

    CHECK(guard.TestDescriptorsState());
}

TEST_CASE("Performance") {
    if constexpr (kBuildType != BuildType::Release) {
        return;
    }
    FileDescriptorsGuard guard;

    auto op = Conjugate(
        [](uint64_t lhs, uint64_t rhs) { return (lhs | 1) * (rhs | 1); },
        Catch::getSeed());

    uint64_t from = 1000;
    uint64_t to = 500'000'000;
    auto par_reduce = RunWithWarmup(
        [op, from, to] { return Reduce(from, to, 1, op, 4); }, 1, 5);

    auto seq_reduce = RunWithWarmup(
        [op, from, to] { return CorrectReduce(from, to, 1, op); }, 1, 5);

    CHECK_SAME_RESULT(from, to, 1, op, 4);

    auto ratio = double(seq_reduce.wall_time.count()) /
                 double(par_reduce.wall_time.count());

    WARN("Parallel version is " << std::fixed << std::setprecision(3) << ratio
                                << " times faster than the ordinary one");
    CHECK(ratio > 3);

    CHECK(guard.TestDescriptorsState());
}

TEST_CASE("NonCommutative") {
    FileDescriptorsGuard guard;

    auto decompose = [](uint64_t value) -> std::pair<uint32_t, uint32_t> {
        return {value >> 32, value};
    };

    auto compose = [decompose](uint64_t lhs, uint64_t rhs) {
        auto [k1, b1] = decompose(lhs);
        auto [k2, b2] = decompose(rhs);
        k1 |= 1;
        k2 |= 1;

        auto k = k1 * k2;
        auto b = k1 * b2 + b1;
        return (uint64_t{k} << 32) | b;
    };

    auto op = Conjugate(std::move(compose), Catch::getSeed());

    CHECK_SAME_RESULT(1000, 100'000'000, 1, op, 4);

    CHECK(guard.TestDescriptorsState());
}
