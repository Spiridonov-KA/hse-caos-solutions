#include "call.hpp"

#include <cstdint>
#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>
#include <functional>

static std::function<int32_t(int32_t, int32_t, int64_t)> real_process;

extern "C" {
int32_t process(int32_t v3, int32_t v1, int64_t v2) {
    return real_process(v3, v1, v2);
}
}

TEST_CASE("Sum") {
    PCGRandom rng{Catch::getSeed()};
    real_process = [](int32_t v3, int32_t v1, int64_t v2) {
        if (v2 > INT32_MAX) {
            return v1 + v3;
        } else {
            return v1;
        }
    };
    for (size_t i = 0; i < 10; ++i) {
        int32_t v1 = static_cast<int32_t>(rng.Generate32()) >> 1;
        int64_t v2 = static_cast<int64_t>(rng.Generate64());
        int32_t v3 = static_cast<int32_t>(rng.Generate32()) >> 1;

        CHECK(process(v3, v1, v2) == Call(v1, v2, v3));
    }
}

TEST_CASE("Linear sum") {
    PCGRandom rng{Catch::getSeed()};
    real_process = [](int32_t v3, int32_t v1, int64_t v2) {
        return 2 * static_cast<int32_t>(v2 >> (32 + 3)) + 3 * v3 + v1;
    };
    for (size_t i = 0; i < 10; ++i) {
        int32_t v1 = static_cast<int32_t>(rng.Generate32()) >> 3;
        int64_t v2 = static_cast<int64_t>(rng.Generate64());
        int32_t v3 = static_cast<int32_t>(rng.Generate32()) >> 3;

        CHECK(process(v3, v1, v2) == Call(v1, v2, v3));
    }
}
