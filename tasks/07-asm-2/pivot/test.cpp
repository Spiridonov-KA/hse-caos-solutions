#include <cstdint>

#include <checksum.hpp>
#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <vector>

uint64_t CalcChecksum(const int16_t* arr, size_t size) {
    UnorderedCheckSum c;
    for (size_t i = 0; i < size; ++i) {
        c.Append(arr[i]);
    }
    return c.GetDigest();
}

extern "C" int Call(int16_t* arr, int size, int pivot);

void IsPivot(int mid, int16_t* arr, size_t size, int pivot,
             uint64_t checksum_before) {
    INFO("Mid = " << mid);
    REQUIRE(0 <= mid);
    REQUIRE(mid < static_cast<int>(size));
    for (int i = 0; i < mid; i++) {
        INFO("Element #" << i << " = " << arr[i]);
        REQUIRE(arr[i] <= pivot);
    }
    for (size_t i = mid; i < size; i++) {
        INFO("Element #" << i << " = " << arr[i]);
        REQUIRE(arr[i] >= pivot);
    }

    auto checksum_after = CalcChecksum(arr, size);
    {
        INFO("Array elements set has changed");
        REQUIRE(checksum_after == checksum_before);
    }
}

TEST_CASE("Simple") {
    constexpr size_t size = 10;
    int16_t arr[size] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    constexpr int pivot = 5;
    auto checksum = CalcChecksum(arr, size);
    IsPivot(Call(arr, size, pivot), arr, size, pivot, checksum);
}

TEST_CASE("SingleElement") {
    int16_t arr[1] = {42};
    int pivot = arr[0];
    int mid = Call(arr, 1, pivot);
    REQUIRE(mid == 0);
    REQUIRE(arr[0] == 42);
}

TEST_CASE("Stress test") {
    PCGRandom rng{Catch::getSeed()};

    for (size_t i = 0; i < 10; ++i) {
        size_t size = rng.Generate32() % 100'000 + 1;
        std::vector<int16_t> arr(size);
        for (size_t i = 0; i < size; i++) {
            arr[i] = static_cast<int16_t>(rng.Generate32() >> 17);
        }
        int pivot = arr[rng.Generate32() % size];
        int mid = Call(arr.data(), size, pivot);
        uint64_t checksum = CalcChecksum(arr.data(), size);
        IsPivot(size_t(mid), arr.data(), size, pivot, checksum);
    }
}
