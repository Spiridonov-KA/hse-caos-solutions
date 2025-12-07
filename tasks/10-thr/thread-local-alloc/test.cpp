#include "storage.hpp"

#include <benchmark/run.hpp>
#include <build.hpp>
#include <check-mt.hpp>
#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

struct MyStat {
    size_t allocation_num{0};
    size_t bytes_allocated{0};

    void Record(size_t size) {
        ++allocation_num;
        bytes_allocated += size;
    }

    bool operator==(const Stat& other) const {
        return allocation_num == other.allocation_num &&
               bytes_allocated == other.bytes_allocated;
    }
};

template <class TStat>
std::ostream& PrintStat(std::ostream& out, const TStat& s) {
    out << "{ allocation_num = " << s.allocation_num
        << ", bytes_allocated = " << s.bytes_allocated << "}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const Stat& s) {
    return PrintStat(out, s);
}

std::ostream& operator<<(std::ostream& out, const MyStat& s) {
    return PrintStat(out, s);
}

TEST_CASE("JustWork") {
    Stat real_stat;
    MyStat expected_stat;
    std::thread([&]() {
        OnThreadStart(&real_stat);
        void* memory = Alloc(10);
        expected_stat.Record(10);
        REQUIRE(memory != nullptr);
        REQUIRE(expected_stat == real_stat);
        OnThreadStop();
    }).join();
}

TEST_CASE("Alignment") {
    PCGRandom rng{424243};
    constexpr std::array<size_t, 5> kAlignments = {1, 2, 4, 8, 16};

    Stat real_stat;
    OnThreadStart(&real_stat);

    std::vector<std::pair<uint64_t, size_t>> allocations;

    for (size_t i = 0; i < 10'000; ++i) {
        auto alignment = kAlignments[rng() % kAlignments.size()];
        auto size = rng() % (alignment << 1) + 1;
        auto ptr = Alloc(size, alignment);
        auto addr = reinterpret_cast<uintptr_t>(ptr);
        INFO("Size: " << size << ", alignment: " << alignment);
        REQUIRE(ptr != nullptr);
        REQUIRE(addr % alignment == 0);
        allocations.emplace_back(addr, size);
    }

    std::ranges::sort(allocations);

    for (size_t i = 0; i + 1 < allocations.size(); ++i) {
        auto [prev_start, prev_size] = allocations[i];
        auto [next_start, next_size] = allocations[i + 1];
#define INTERVAL(start, size) "[" << start << ", " << start + size << ")"
        INFO("Previous allocation: " << INTERVAL(prev_start, prev_size));
        INFO("Next allocation: " << INTERVAL(next_start, next_size));
#undef INTERVAL
        REQUIRE(prev_start + prev_size <= next_start);
    }

    OnThreadStop();
}

TEST_CASE("NotWasteful") {
    Stat real_stat;
    OnThreadStart(&real_stat);

    for (size_t i = 0; i < 1024 * 512; ++i) {
        void* ptr = Alloc(1, 1);
        REQUIRE(ptr != nullptr);
        *reinterpret_cast<char*>(ptr) = 123;
    }

    OnThreadStop();
}

TEST_CASE("AllocatorPerformance") {
    if constexpr (kBuildType != BuildType::Release) {
        return;
    }

    constexpr size_t kAllocations = 1024 * 512;
    Stat real_stat;
    OnThreadStart(&real_stat);

    auto alloc_performance = RunWithWarmup(
        []() {
            for (size_t i = 0; i < kAllocations; ++i) {
                Alloc(1, 1);
            }
            Dealloc();
        },
        1, 5);

    auto increment_performance = RunWithWarmup(
        []() {
            volatile size_t cnt = 0;
            for (size_t i = 0; i < kAllocations; ++i) {
                cnt = cnt + 1;
            }
        },
        1, 5);

    REQUIRE(alloc_performance.cpu_time < increment_performance.cpu_time * 4);

    OnThreadStop();
}

TEST_CASE("WorksMultiple") {
    constexpr size_t kThreads = 10;

    std::vector<MyStat> expected_stat(kThreads);
    std::vector<Stat> real_stats(kThreads);
    std::vector<std::jthread> threads;

    for (size_t i = 0; i < kThreads; i++) {
        threads.emplace_back([&, i]() {
            OnThreadStart(&real_stats[i]);
            void* memory = Alloc(i + 1);
            expected_stat[i].Record(i + 1);
            REQUIRE_MT(memory != nullptr);
            REQUIRE_MT(expected_stat[i] == real_stats[i]);
            OnThreadStop();
        });
    }
    for (int i = 0; i < 10; i++) {
        threads[i].join();
        REQUIRE_MT(expected_stat[i] == real_stats[i]);
    }
}

struct ListNode {
    ListNode* prev;
    ListNode* next;

    int value;
};

static ListNode* Construct(const std::vector<int>& values, Stat& stat,
                           bool check_construct = true) {
    ListNode* head = nullptr;
    ListNode* tail = nullptr;

    MyStat my_stat{};

    for (size_t i = 0; i < values.size(); ++i) {
        my_stat.Record(sizeof(ListNode));
        auto node = reinterpret_cast<ListNode*>(
            Alloc(sizeof(ListNode), alignof(ListNode)));
        if (check_construct) {
            REQUIRE_MT(node != nullptr);
            REQUIRE_MT(stat == my_stat);
        }

        node->next = nullptr;
        node->prev = tail;
        node->value = values[i];
        if (tail != nullptr) {
            tail->next = node;
        }
        tail = node;
        if (head == nullptr) {
            head = tail;
        }
    }
    return head;
}

static ListNode* Reverse(ListNode* head) {
    ListNode* prev = nullptr;
    while (head != nullptr) {
        auto next = head->next;
        head->prev = next;
        head->next = prev;
        prev = head;
        head = next;
    }
    return prev;
}

void CheckListWorks(const std::vector<int>& values, Stat& stat,
                    bool check = true) {
    auto head = Construct(values, stat, check);
    head = Reverse(head);
    bool all_checks_passed = true;
    for (auto val = values.rbegin(); val < values.rend(); ++val) {
        if (*val != head->value) {
            all_checks_passed = false;
        }
        head = head->next;
    }
    if (check) {
        REQUIRE_MT(all_checks_passed);
    }
}

TEST_CASE("CheckContention") {
    if constexpr (kBuildType != BuildType::Release) {
        return;
    }

    PCGRandom rng{42};

    constexpr size_t kLength = 20'000;
    std::vector<std::vector<int>> values(4);

    for (size_t i = 0; i < kLength; i++) {
        auto val = rng.Generate32();
        for (size_t j = 0; j < 4; j++) {
            values[j].push_back(val);
        }
    }

    auto run_threads = [&values](size_t threads_count, bool check) {
        constexpr size_t epochs_count = 100;
        std::vector<Stat> real_stats(threads_count);
        std::vector<std::jthread> threads;
        for (size_t i = 0; i < threads_count; i++) {
            threads.emplace_back([&, i]() {
                OnThreadStart(&real_stats[i]);
                for (size_t j = 0; j < epochs_count; j++) {
                    CheckListWorks(values[i], real_stats[i], check);
                    Dealloc();
                }
                OnThreadStop();
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    };

    // test works
    for (size_t threads_count : {1, 4}) {
        run_threads(threads_count, true);
    }

    auto one_thread = RunWithWarmup(
        [run_threads] {
            run_threads(1, false);
        },
        2, 5);

    auto four_threads = RunWithWarmup(
        [run_threads] {
            run_threads(4, false);
        },
        2, 5);

    auto ratio = double(four_threads.wall_time.count()) /
                 double(one_thread.wall_time.count());

    WARN("1-thread version is " << std::fixed << std::setprecision(3) << ratio
                                << " times faster than the 4-thread one");
    CHECK(ratio < 1.45);
}
