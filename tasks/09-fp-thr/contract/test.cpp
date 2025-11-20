#include "contract.hpp"

#include <backoff.hpp>
#include <pcg-random.hpp>
#include <step-thread-runner.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>

using namespace std::chrono_literals;

TEST_CASE("JustWorks") {
    auto [f, p] = CreateContract<int>();

    std::move(p).Set(123);

    auto v = std::move(f).Get();
    REQUIRE(v == 123);
}

TEST_CASE("Movable") {
    auto [f, p] = CreateContract<int>();

    auto f2 = std::move(f);
    auto p2 = std::move(p);

    std::move(p2).Set(543);
    auto value = std::move(f2).Get();
    REQUIRE(value == 543);
}

TEST_CASE("MultiThreaded") {
    PCGRandom rng{Catch::getSeed()};

    StepThreadRunner r;
    auto [f, p] = CreateContract<uint64_t>();
    uint64_t value;
    r.Add(
        [&rng, &value, &p] {
            value = rng.Generate64();
            auto p_local = std::move(p);
            std::move(p_local).Set(value);
        },
        500ms);
    r.Add(
        [&value, &f] {
            auto f_local = std::move(f);
            auto v = std::move(f_local).Get();
            CHECK(v == value);
        },
        500ms);
    while (r.DoStep()) {
        std::tie(f, p) = CreateContract<uint64_t>();
    }
    std::move(r).Join();
}
