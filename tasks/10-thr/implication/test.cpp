#include "implication.hpp"

#include <benchmark/run.hpp>
#include <build.hpp>
#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <iomanip>

static bool SequentialImplication(const std::vector<bool>& input) {
    bool res = true;
    for (size_t i = 0; i < input.size(); ++i) {
        res = (!res || input[i]);
    }
    return res;
}

TEST_CASE("JustWorks") {
    REQUIRE(Implication({0}) == 0);
    REQUIRE(Implication({1}) == 1);
    REQUIRE(Implication({1, 0}) == 0);
    REQUIRE(Implication({1, 1, 0}) == 0);
    REQUIRE(Implication({1, 1, 1}) == 1);
    REQUIRE(Implication({1, 0, 1}) == 1);
}

TEST_CASE("StressTest") {
    PCGRandom rng{Catch::getSeed()};

    for (auto max_len : {10, 1'000, 10'000, 100'000'000}) {
        int cnt = 100;
        if (max_len == 100'000'000) {
            if constexpr (kBuildType != BuildType::Release) {
                continue;
            }
            cnt = 5;
        }
        for (int i = 0; i < cnt; i++) {
            size_t length = rng.Generate64() % max_len + 1;
            std::vector<bool> input(length);
            for (size_t i = 0; i < length; i++) {
                input[i] = rng.Generate64() % 2;  // Slow but sure
            }
            REQUIRE(SequentialImplication(input) == Implication(input));
        }
    }
}

TEST_CASE("Performance") {
    if constexpr (kBuildType != BuildType::Release) {
        return;
    }

    PCGRandom rng{42};

    constexpr size_t length = 100'000'000;
    std::vector<bool> input(length);
    for (size_t i = 0; i < length; i++) {
        input[i] = rng.Generate64() % 2;  // Slow but sure
    }

    REQUIRE(SequentialImplication(input) == Implication(input));

    auto seq = RunWithWarmup(
        [&] {
            return SequentialImplication(input);
        },
        2, 5);

    auto par = RunWithWarmup(
        [&] {
            return Implication(input);
        },
        2, 5);

    auto ratio = double(seq.wall_time.count()) / double(par.wall_time.count());

    WARN("Parallel version is " << std::fixed << std::setprecision(3) << ratio
                                << " times faster than the ordinary one");
    CHECK(ratio > 3);
}
