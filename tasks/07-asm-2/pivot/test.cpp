#include <cstdint>

#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <vector>

extern "C" int Call(int16_t* arr, int size, int pivot);

void IsPivot(int mid, int16_t* arr, int size, int pivot) {
    INFO("Mid = " << mid);
    REQUIRE(0 <= mid);
    REQUIRE(mid < size);
    for (int i = 0; i < mid; i++) {
        INFO("Element #" << i << " = " << arr[i]);
        CHECK(arr[i] <= pivot);
    }
    for (int i = mid; i < size; i++) {
        INFO("Element #" << i << " = " << arr[i]);
        CHECK(arr[i] >= pivot);
    }
}

TEST_CASE("Simple") {
    constexpr size_t size = 10;
    int16_t arr[size] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    constexpr int pivot = 5;
    IsPivot(Call(arr, size, pivot), arr, size, pivot);
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
        IsPivot(Call(arr.data(), size, pivot), arr.data(), size, pivot);
    }
}
