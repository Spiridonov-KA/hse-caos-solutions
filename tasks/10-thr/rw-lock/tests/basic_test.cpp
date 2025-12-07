#include "../rw-lock.hpp"
#include "common.hpp"

#include <benchmark/timer.hpp>
#include <build.hpp>
#include <thread-runner.hpp>

#include <atomic>
#include <random>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

TEST_CASE("OnlyReaders") {
    RWLock m;

    std::vector<std::thread> th;
    for (size_t i = 0; i < kThreads; ++i) {
        th.emplace_back([&]() {
            for (int j = 0; j < 10000; ++j) {
                m.ReadLock();
                m.ReadUnlock();
            }
        });
    }

    for (auto& t : th) {
        t.join();
    }
}

template <class T>
inline T FetchMax(std::atomic<T>& a, T value) {
    T prev = a.load();
    while (prev < value && !a.compare_exchange_weak(prev, value)) {
    }
    return prev;
}

TEST_CASE("OnlyReadersTryLock") {
    RWLock m;

    std::vector<std::thread> th;
    std::atomic<uint32_t> max_parallelism{0};
    CriticalSectionState state;

    for (size_t i = 0; i < kThreads; ++i) {
        th.emplace_back([&]() {
            for (int j = 0; j < 10; ++j) {
                auto success = m.ReadTryLock();
                REQUIRE_MT(success);
                {
                    auto [_guard, s] = state.Reader();
                    FetchMax(max_parallelism, s.readers);
                    std::this_thread::sleep_for(1ms);
                }
                m.ReadUnlock();
            }
        });
    }

    for (auto& t : th) {
        t.join();
    }
    REQUIRE(max_parallelism.load() >= kThreads - 2);
}

TEST_CASE("OnlyWriters") {
    RWLock m;
    CriticalSectionState state;

    std::vector<std::thread> th;
    for (size_t i = 0; i < kThreads; ++i) {
        th.emplace_back([&]() {
            for (int j = 0; j < 30; ++j) {
                m.WriteLock();
                {
                    auto [_guard, _state] = state.Writer();
                    std::this_thread::sleep_for(1ms);
                }
                m.WriteUnlock();
            }
        });
    }

    for (auto& t : th) {
        t.join();
    }
}

TEST_CASE("ReadersAndWriters") {
    RWLock m;
    CriticalSectionState state;
    std::atomic<uint32_t> max_parallelism{0};

    std::vector<std::thread> th;

    for (size_t i = 0; i < kReaders; ++i) {
        th.emplace_back([&]() {
            for (int k = 0; k < 1'000; ++k) {
                m.ReadLock();
                {
                    auto [_guard, s] = state.Reader();
                    FetchMax(max_parallelism, s.readers);
                    std::this_thread::sleep_for(100us);
                }
                m.ReadUnlock();
            }
        });
    }

    for (size_t i = 0; i < kWriters; ++i) {
        th.emplace_back([&]() {
            for (int k = 0; k < 1'000; ++k) {
                m.WriteLock();
                {
                    auto [_guard, _state] = state.Writer();
                    std::this_thread::sleep_for(100us);
                }
                m.WriteUnlock();
            }
        });
    }

    for (auto& t : th) {
        t.join();
    }
}

TEST_CASE("TryLockTests") {
    RWLock m;

    bool success;

    success = m.WriteTryLock();
    REQUIRE(success);

    success = m.WriteTryLock();
    REQUIRE(!success);

    m.WriteUnlock();

    success = m.ReadTryLock();
    REQUIRE(success);

    success = m.WriteTryLock();
    REQUIRE(!success);

    success = m.ReadTryLock();
    REQUIRE(success);

    success = m.ReadTryLock();
    REQUIRE(success);

    m.ReadUnlock();
    m.ReadUnlock();
    m.ReadUnlock();
}

TEST_CASE("ReaderWriterCorrectnessCounter") {
    RWLock m;
    int shared_value = 0;
    std::atomic<bool> stop = false;

    std::thread writer([&]() {
        for (int i = 0; i < 20000; ++i) {
            m.WriteLock();
            shared_value++;
            m.WriteUnlock();
        }
        stop.store(true);
    });

    std::thread reader([&]() {
        int last = 0;
        while (!stop.load()) {
            m.ReadLock();
            int v = shared_value;
            REQUIRE_MT(v >= last);
            last = v;
            m.ReadUnlock();
        }
    });

    writer.join();
    reader.join();
}

TEST_CASE("StressTest") {
    RWLock m;
    CriticalSectionState state;

    std::vector<std::thread> th;

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(0, 9);

    for (size_t i = 0; i < kThreads; ++i) {
        th.emplace_back([&, i]() {
            std::mt19937 r(i + 1);

            for (int iter = 0; iter < 20000; ++iter) {
                int op = dist(r);  // 0–6 read, 7–9 write

                if (op < 7) {
                    // read
                    m.ReadLock();
                    {
                        auto [_guard, _state] = state.Reader();
                    }
                    m.ReadUnlock();
                } else {
                    // write
                    m.WriteLock();
                    {
                        auto [_guard, _state] = state.Writer();
                    }
                    m.WriteUnlock();
                }
            }
        });
    }

    for (auto& t : th) {
        t.join();
    }
}

static void WarmupLock(RWLock& lk) {
    ThreadRunner t;
    for (size_t i = 0; i < 2; ++i) {
        t.Run(
            [&lk](auto) {
                lk.WriteLock();
                lk.WriteUnlock();
            },
            100ms);
    }
    for (size_t i = 0; i < 2; ++i) {
        t.Run(
            [&lk](auto) {
                lk.ReadLock();
                lk.ReadUnlock();
            },
            100ms);
    }

    std::move(t).Join();
}

TEST_CASE("DoNotBurnCPU") {
    if constexpr (kBuildType != BuildType::Release) {
        return;
    }
    RWLock lk;
    WarmupLock(lk);

    std::array<
        std::tuple<void (RWLock::*)(), void (RWLock::*)(), std::string_view>, 2>
        methods = {{
            {&RWLock::WriteLock, &RWLock::WriteUnlock, "write"},
            {&RWLock::ReadLock, &RWLock::ReadUnlock, "read"},
        }};

    for (size_t cnt = 0; cnt < 3; ++cnt) {
        for (size_t i = 0; i < 2; ++i) {
            for (size_t j = 0; j < 2 - i; ++j) {
                auto [lock1, unlock1, kind1] = methods[i];
                auto [lock2, unlock2, kind2] = methods[j];

                CPUTimer::Times times;

                (lk.*lock1)();
                std::thread waiter([&]() {
                    CPUTimer timer(CPUTimer::Thread);
                    (lk.*lock2)();
                    (lk.*unlock2)();
                    times = timer.GetTimes();
                });

                std::this_thread::sleep_for(50ms);
                (lk.*unlock1)();
                waiter.join();
                INFO("Lock was blocked by " << kind1 << "er, lock attempted by "
                                            << kind2 << "er");
                REQUIRE(times.cpu_time < 3ms);
            }
        }
    }
}
