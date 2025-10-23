#include "solution.hpp"

#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Works") {
    PCGRandom rng{Catch::getSeed()};
    for (size_t i = 0; i < 10; ++i) {
#ifdef USE_WRONG_1
        uint64_t v1 = rng.Generate64() >> 2;
        uint64_t v2 = rng.Generate64() >> 2;
        uint64_t v3 = rng.Generate64() >> 2;

        CHECK(Solve(v1, v2, v3) == v1 + v2 + v3);
#else
        uint64_t v1 = rng.Generate64() >> 3;
        uint64_t v2 = rng.Generate64() >> 3;
        uint64_t v3 = rng.Generate64() >> 3;
        uint64_t v4 = rng.Generate64() >> 3;
        uint64_t v5 = rng.Generate64() >> 3;
        uint64_t v6 = rng.Generate64() >> 3;
        uint64_t v7 = rng.Generate64() >> 3;

        CHECK(Solve(v1, v2, v3, v4, v5, v6, v7) ==
              v1 + v2 + v3 + v4 + v5 + v6 + v7);
#endif
    }
}
