#include "buffer.hpp"

#include <mm.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <random>
#include <type_traits>

TEST_CASE("DataType") {
    static_assert(std::is_same_v<RingBuffer::ElementType, int64_t>);
    static_assert(std::is_same_v<decltype(std::declval<RingBuffer>().Data()),
                                 std::span<RingBuffer::ElementType>>);
}

struct MediocreRingBuffer {
    using ElementType = RingBuffer::ElementType;

    MediocreRingBuffer(size_t capacity) : buf_(capacity) {
    }

    void PushBack(ElementType value) {
        buf_[end_++] = value;
        if (end_ == buf_.size()) {
            end_ = 0;
        }
    }

    void PopBack() {
        if (end_ == 0) {
            end_ = buf_.size();
        }
        --end_;
    }

    void PushFront(ElementType value) {
        if (begin_ == 0) {
            begin_ = buf_.size();
        }
        buf_[--begin_] = value;
    }

    void PopFront() {
        ++begin_;
        if (begin_ == buf_.size()) {
            begin_ = 0;
        }
    }

    ElementType& operator[](size_t idx) {
        auto internal_idx = begin_ + idx;
        if (internal_idx >= buf_.size()) {
            internal_idx -= buf_.size();
        }
        return buf_[internal_idx];
    }

  private:
    std::vector<ElementType> buf_;
    size_t begin_ = 0;
    size_t end_ = 0;
};

RingBuffer CreateBuffer(size_t capacity) {
    auto result = RingBuffer::Create(capacity);
    REQUIRE(result.index() == 0);
    return std::move(*std::get_if<RingBuffer>(&result));
}

TEST_CASE("JustWorks") {
    auto buf = CreateBuffer(123);
    CHECK(buf.Capacity() <= 1024);
    buf.PushBack(1);
    buf.PushBack(2);
    buf.PushBack(3);

    std::array expected = {1, 2, 3};
    auto data = buf.Data();
    REQUIRE(data.size() == expected.size());
    for (size_t i = 0; i < data.size(); ++i) {
        INFO("Checking " << i);
        CHECK(data[i] == expected[i]);
    }
}

TEST_CASE("Wraps") {
    auto buf = CreateBuffer(512);
    CHECK(buf.Capacity() < 1024);
    CHECK(buf.Capacity() >= 512);

    auto cap = buf.Capacity();

    RingBuffer::ElementType elem = 0;
    for (size_t i = 0; i < 2 * cap / 3; ++i) {
        buf.PushBack(elem++);
        REQUIRE(buf.Data().size() == i + 1);
    }

    for (size_t i = 0; i < cap; ++i) {
        buf.PopFront();
        buf.PushBack(elem++);
    }

    {
        auto data = buf.Data();
        auto start = elem - static_cast<RingBuffer::ElementType>(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            INFO("Checking elem " << i);
            CHECK(data[i] == start + static_cast<RingBuffer::ElementType>(i));
        }
    }
}

void StressTestBuffer(std::mt19937_64& rng, RingBuffer& buf,
                      MediocreRingBuffer& model, size_t iterations) {
    auto cap = buf.Capacity();
    size_t size = buf.Data().size();

    for (size_t i = 0; i < iterations; ++i) {
        REQUIRE(buf.Data().size() == size);

        if (rng() & 1) {
            auto value = static_cast<RingBuffer::ElementType>(rng());
            if (size != cap && (size == 0 || (rng() & 1))) {
                if (rng() & 1) {
                    buf.PushBack(value);
                    model.PushBack(value);
                } else {
                    buf.PushFront(value);
                    model.PushFront(value);
                }
                ++size;
            } else {
                if (rng() & 1) {
                    buf.PopBack();
                    model.PopBack();
                } else {
                    buf.PopFront();
                    model.PopFront();
                }
                --size;
            }
        } else {
            if (size == 0) {
                continue;
            }

            auto index = rng() % size;
            if (rng() & 1) {
                auto value = static_cast<RingBuffer::ElementType>(rng());
                buf.Data()[index] = value;
                model[index] = value;
            } else {
                INFO("Probing element " << index << " on iteration " << i);
                REQUIRE(buf.Data()[index] == model[index]);
            }
        }
    }
}

void StressTestInitialCapacity(std::mt19937_64& rng, size_t initial_capacity,
                               size_t iterations) {
    auto buf = CreateBuffer(initial_capacity);
    auto cap = buf.Capacity();
    CHECK(cap < std::max(kPageSize / sizeof(RingBuffer::ElementType) + 1,
                         2 * initial_capacity));
    CHECK(cap >= initial_capacity);
    MediocreRingBuffer model{cap};

    StressTestBuffer(rng, buf, model, iterations);
}

TEST_CASE("StressTest") {
    std::mt19937_64 rng{Catch::getSeed()};
    static constexpr size_t kIterations = 100'000;

    SECTION("100") {
        StressTestInitialCapacity(rng, 100, kIterations);
    }

    SECTION("1000") {
        StressTestInitialCapacity(rng, 1'000, kIterations);
    }

    SECTION("10000") {
        StressTestInitialCapacity(rng, 10'000, kIterations);
    }
}

TEST_CASE("MemoryReleased") {
    uintptr_t begin_page;
    uintptr_t end_page;

    {
        auto buf = CreateBuffer(2'000);
        buf.PushBack(1);

        RingBuffer::ElementType* begin = buf.Data().data();
        RingBuffer::ElementType* end = begin + buf.Data().size();

        MediocreRingBuffer model{buf.Capacity()};
        model.PushBack(1);

        std::mt19937_64 rng{Catch::getSeed()};

        for (size_t i = 0; i < 10000; ++i) {
            INFO("Outer iteration " << i);
            StressTestBuffer(rng, buf, model, 10);
            auto data = buf.Data();
            if (data.size() == 0) {
                continue;
            }

            if (data.data() < begin) {
                begin = data.data();
            }
            if (auto cur_end = data.data() + data.size(); cur_end > end) {
                end = cur_end;
            }
        }

        {
            INFO("Buffer's content is not stationary");
            CHECK(static_cast<size_t>(end - begin) <= buf.Capacity() * 2);
        }

        begin_page = uintptr_t(begin) & ~(kPageSize - 1);
        end_page = (uintptr_t(end) + kPageSize - 1) & ~(kPageSize - 1);
        for (auto page = begin_page; page < end_page; page += kPageSize) {
            auto addr = reinterpret_cast<void*>(page);
            INFO("Page at address " << addr << " is not valid");
            CHECK(IsValidPage(addr));
        }
    }

    for (auto page = begin_page; page < end_page; page += kPageSize) {
        auto addr = reinterpret_cast<void*>(page);
        INFO("Page at address " << addr << " is not released");
        CHECK(!IsValidPage(addr));
    }
}
