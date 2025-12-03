#include "limbo.hpp"

#include <build.hpp>
#include <check-mt.hpp>
#include <step-thread-runner.hpp>
#include <thread-runner.hpp>

#include <benchmark/timer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>  // IWYU pragma: keep

using namespace std::chrono_literals;

TEST_CASE("SingleThread") {
    Limbo l;

    l.Wait(0);
    l.Wait(-10);
    l.Raise(100);
    l.Wait(99);
    l.Wait(100);
}

TEST_CASE("MessagePassing") {
    StepThreadRunner r(200ms);

    int message = 0;
    Limbo l;

    r.Add([&]() {
        message = 1;
        l.Raise(1);
    });

    r.Add([&]() {
        l.Wait(1);
        CHECK_MT(message == 1);
    });

    while (r.DoStep()) {
        message = 0;
        l.Raise(0);
    }
}

template <class T>
struct CopyableAtomic : std::atomic<T> {
    using std::atomic<T>::atomic;

    CopyableAtomic(const CopyableAtomic& other) : CopyableAtomic(other.load()) {
    }

    CopyableAtomic(CopyableAtomic&& other) : CopyableAtomic(other.load()) {
    }
};

TEST_CASE("WakeExact") {
    constexpr int kThreads = 20;

    Limbo l;
    std::vector<std::jthread> r;
    std::vector<CopyableAtomic<bool>> finished(kThreads, false);
    std::vector<bool> shared(kThreads, false);

    for (int i = 0; i < kThreads; ++i) {
        r.emplace_back([i, &l, &shared, &finished]() {
            l.Wait(i / 2 + 1);
            CHECK_MT(shared[i]);
            finished[i].store(true);
        });
    }

    std::this_thread::sleep_for(2ms);
    for (int i = 0; i < kThreads / 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            CHECK_MT(!finished[i * 2 + j].load());
            shared[i * 2 + j] = true;
        }
        l.Raise(i + 1);
        std::this_thread::sleep_for(10ms);
        for (int j = 0; j < 2; ++j) {
            INFO("i = " << i << ", j = " << j);
            CHECK_MT(finished[i * 2 + j].load());
        }
    }
}

TEST_CASE("DoNotBurnCPU") {
    if constexpr (kBuildType != BuildType::Release) {
        return;
    }

    Limbo l;

    CPUTimer timer;
    std::thread t([&] {
        std::this_thread::sleep_for(100ms);
        l.Raise(10);
    });

    l.Wait(7);
    auto times = timer.GetTimes();
    t.join();

    REQUIRE(times.wall_time >= 100ms);
    REQUIRE(times.TotalCpuTime() < 2ms);
}

TEST_CASE("NoExtraWakeups") {
    if constexpr (kBuildType != BuildType::Release) {
        return;
    }

    constexpr size_t kThreads = 100;
    constexpr int kIterations = 50'000;
    constexpr int kWaitStart = 2'000'000;
    Limbo l;
    std::vector<std::jthread> waiters(kThreads);
    std::vector<CPUTimer::Times> times(kThreads);
    std::atomic<uint64_t> finished{0};

    for (size_t i = 0; i < kThreads; ++i) {
        waiters[i] = std::jthread([&, i]() {
            CPUTimer timer(CPUTimer::Thread);
            l.Wait(static_cast<int>(i) + kWaitStart);
            times[i] = timer.GetTimes();
            finished.fetch_add(1);
        });
    }

    std::jthread stepper([&] {
        for (int i = 0; i < kIterations; ++i) {
            l.Wait(i);
        }
    });

    for (int i = 0; i < kIterations; ++i) {
        l.Raise(i);
        std::this_thread::sleep_for(500ns);
    }

    CHECK(finished.load() == 0);

    l.Raise(kWaitStart + static_cast<int>(kThreads));

    for (auto& w : waiters) {
        w.join();
    }
    stepper.join();

    CPUTimer::Times total;
    for (auto& x : times) {
        total += x;
    }

    REQUIRE(total.TotalCpuTime() < 20ms);
}
