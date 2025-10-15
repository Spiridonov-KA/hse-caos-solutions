#include "arg-sum-1.hpp"

#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#define CHECK_ARG_SUM(a, b, c, d) CHECK(ArgSum(a, b, c, d) == a + b + c + d)

TEST_CASE("ArgSum") {
    PCGRandom rng{Catch::getSeed()};
    CHECK_ARG_SUM(1, 2, 3, 4);

    constexpr size_t kValues = 7;
    std::array<uint64_t, kValues> values;
    for (auto& x : values) {
        x = rng.Generate64();
    }

    CHECK_ARG_SUM(values[0], values[1], values[2], values[3]);

    for (size_t i = 0; i < 10; ++i) {
        size_t a = rng() % kValues;
        size_t b = rng() % kValues;
        size_t c = rng() % kValues;
        size_t d = rng() % kValues;
        CHECK_ARG_SUM(values[a], values[b], values[c], values[d]);
        values[rng() % kValues] = rng.Generate64();
    }
}
