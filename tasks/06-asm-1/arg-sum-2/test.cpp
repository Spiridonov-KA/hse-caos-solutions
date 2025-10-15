#include "arg-sum-2.hpp"

#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("ArgSumShort") {
    PCGRandom rng{Catch::getSeed()};
    uint64_t a = 123;
    uint64_t* ptr = &a;
    CHECK(ArgSumShort(a, &a, &ptr) == 3 * a);

    a = rng.Generate64();
    uint64_t b = rng.Generate64();
    uint64_t c = rng.Generate64();

    ptr = &c;
    CHECK(ArgSumShort(a, &b, &ptr) == a + b + c);

    c = rng.Generate64();
    ptr = &b;
    CHECK(ArgSumShort(c, &a, &ptr) == a + b + c);
}

TEST_CASE("ArgSumLong") {
    PCGRandom rng{Catch::getSeed(), 123};
    auto arg_sum = [&rng](uint64_t a, uint64_t* b, uint64_t** c) {
        return ArgSumLong(rng.Generate64(), rng.Generate64(), rng.Generate64(),
                          rng.Generate64(), rng.Generate64(), a, b, c);
    };

    uint64_t a = 321;
    uint64_t* ptr = &a;
    CHECK(arg_sum(a, &a, &ptr) == 3 * a);

    uint64_t b = rng.Generate64();
    uint64_t c = rng.Generate64();

    CHECK(arg_sum(c, &b, &ptr) == a + b + c);

    a = rng.Generate64();
    c = rng.Generate64();
    ptr = &c;
    CHECK(arg_sum(a, &c, &ptr) == a + 2 * c);
}
