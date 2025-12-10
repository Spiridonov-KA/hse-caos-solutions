#include "table.hpp"

#include <benchmark/timer.hpp>
#include <thread-runner.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace std::chrono_literals;

void StressTest(size_t n, std::chrono::nanoseconds duration) {
    INFO("n = " << n << ", duration = " << duration);
    Table t{n};
    ThreadRunner r;

    for (size_t i = 0; i < n; ++i) {
        r.Run(
            [i, &t](auto) {
                std::this_thread::yield();
                t.StartEating(i);
                std::this_thread::yield();
                t.StopEating(i);
            },
            duration);
    }

    std::move(r).Join();
}

TEST_CASE("StressTest") {
    StressTest(4, 500ms);
}

TEST_CASE("JustWorks") {
    Table t{4};

    t.StartEating(0);
    t.StartEating(2);

    t.StopEating(0);
    t.StopEating(2);

    t.StartEating(1);
    t.StopEating(1);

    t.StartEating(3);
    t.StopEating(3);
}

TEST_CASE("DoNotBurnCPU") {
    constexpr size_t kPhilosophers = 4;
    Table t{kPhilosophers};
    ThreadRunner r;

    for (size_t i = 0; i < kPhilosophers; ++i) {
        for (size_t side = 0; side < 2; ++side) {
            // Hungry philosopher
            r.Run(
                [&t, i](auto) {
                    t.StartEating(i);
                    // Feasts for 100ms
                    std::this_thread::sleep_for(100ms);
                    t.StopEating(i);
                },
                100ms);

            size_t j = kPhilosophers + i + 2 * side - 1;
            j %= kPhilosophers;
            std::this_thread::sleep_for(1ms);

            CPUTimer timer{CPUTimer::Thread};

            t.StartEating(j);
            t.StopEating(j);

            auto times = timer.GetTimes();
            REQUIRE(times.cpu_time < 1ms);
        }
    }

    std::move(r).Join();
}

TEST_CASE("ProtectsData") {
    constexpr size_t kPhilosophers = 4;
    Table t{kPhilosophers};
    ThreadRunner r;
    std::array<uint64_t, kPhilosophers> shared_data;

    for (size_t i = 0; i < kPhilosophers; ++i) {
        r.Run(
            [&t, &shared_data, i](auto) {
                std::this_thread::yield();
                t.StartEating(i);
                shared_data[i] = i;
                std::this_thread::yield();
                shared_data[(i + 1) % kPhilosophers] = i;
                t.StopEating(i);
            },
            200ms);
    }

    std::move(r).Join();
}

TEST_CASE("AllSizes") {
    constexpr size_t kUpTo = 5;

    for (size_t i = 2; i <= kUpTo; ++i) {
        Table t{i};
        ThreadRunner r;

        for (size_t j = 0; j < i; ++j) {
            r.Run(
                [&t, j](auto) {
                    std::this_thread::yield();
                    t.StartEating(j);
                    std::this_thread::yield();
                    t.StopEating(j);
                },
                50ms);
        }

        std::move(r).Join();
    }
}
