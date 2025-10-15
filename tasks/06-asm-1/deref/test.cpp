#include "deref.hpp"

#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <type_traits>

template <class T>
size_t CountPtrs(T value) {
    if constexpr (std::is_pointer_v<T>) {
        return CountPtrs(*value) + 1;
    } else {
        return 0;
    }
}

template <class T>
uint64_t Deref(T value) {
    return Deref(reinterpret_cast<void*>(value), CountPtrs(value));
}

TEST_CASE("Simple") {
    PCGRandom rng{Catch::getSeed()};
    CHECK(Deref(uint64_t(1)) == 1);
    CHECK(Deref(uint64_t(0)) == 0);

    uint64_t a = rng.Generate64();
    CHECK(Deref(&a) == a);

    uint64_t* ptr = &a;
    a = rng.Generate64();
    CHECK(Deref(&ptr) == a);

    uint64_t** ptr2 = &ptr;
    a = rng.Generate64();
    CHECK(Deref(&ptr2) == a);
}

template <size_t D, class T>
void CheckDepth(T value, uint64_t expected) {
    CHECK(Deref(value) == expected);

    if constexpr (D != 0) {
        CheckDepth<D - 1>(&value, expected);
    }
}

TEST_CASE("Nested") {
    PCGRandom rng{Catch::getSeed(), 1023};
    uint64_t a = rng();
    CheckDepth<5>(a, a);
    CheckDepth<5>(uint64_t(0), 0);

    CheckDepth<10>(&a, a);

    CheckDepth<100>(&a, a);
    CheckDepth<100>(uint64_t(0), 0);
}

TEST_CASE("Path") {
    constexpr size_t kNodes = 50'000;
    PCGRandom rng{Catch::getSeed()};

    std::vector<void*> nodes(kNodes);
    std::vector<size_t> indices(kNodes);
    std::ranges::iota(indices, 0);
    std::ranges::shuffle(indices, rng);

    auto target = rng.Generate64();
    nodes[indices[0]] = reinterpret_cast<void*>(target);
    for (size_t i = 0; i + 1 < kNodes; ++i) {
        nodes[indices[i + 1]] = &nodes[indices[i]];
    }

    CHECK(Deref(&nodes[indices.back()], kNodes) == target);
}
