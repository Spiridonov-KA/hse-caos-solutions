#include "queue.hpp"

#include <benchmark/timer.hpp>
#include <build.hpp>
#include <checksum.hpp>
#include <pcg-random.hpp>
#include <step-thread-runner.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace std::chrono_literals;

TEST_CASE("SingleThread") {
    Queue<int> q;
    q.Push(123);
    q.Push(456);
    q.Push(789);

    {
        auto value = q.Pop();
        REQUIRE(value == 123);
    }
    {
        auto value = q.Pop();
        REQUIRE(value == 456);
    }
    {
        auto value = q.Pop();
        REQUIRE(value == 789);
    }

    q.Close();
    {
        auto value = q.Pop();
        REQUIRE(value == std::nullopt);
    }
}

TEST_CASE("DeadlockPop") {
    Queue<int> q;
    StepThreadRunner r{400ms};

    std::atomic<bool> had_nullopt{false};

    for (size_t i = 0; i < 3; ++i) {
        r.Add([&q, &had_nullopt]() {
            auto v = q.Pop();
            if (!v) {
                had_nullopt.store(true);
            }
        });
    }

    for (size_t i = 0; i < 3; ++i) {
        r.Add([&q]() {
            q.Push(123);
        });
    }

    while (r.DoStep()) {
    }

    REQUIRE(!had_nullopt.load());
}

TEST_CASE("DeadlockClose") {
    Queue<int>* q;
    StepThreadRunner r{400ms};

    std::atomic<bool> had_value{false};

    for (size_t i = 0; i < 3; ++i) {
        r.Add([&q, &had_value]() {
            auto v = q->Pop();
            if (v) {
                had_value.store(true);
            }
        });
    }

    r.Add([&q]() {
        q->Close();
    });

    while (true) {
        Queue<int> queue;
        q = &queue;
        if (!r.DoStep()) {
            break;
        }
    }

    REQUIRE(!had_value.load());
}

TEST_CASE("DoNotBurnCPU") {
    Queue<int> q;
    std::thread t([&q]() {
        std::this_thread::sleep_for(100ms);
        q.Push(123);
    });

    CPUTimer timer{CPUTimer::Thread};
    q.Pop();
    auto times = timer.GetTimes();
    t.join();

    REQUIRE(times.cpu_time < 1ms);
}

TEST_CASE("ValuesMatch") {
    constexpr size_t kProducers = 3;
    constexpr size_t kConsumers = 3;

    Queue<uint64_t> q;
    PCGRandom rng{Catch::getSeed()};
    ThreadRunner r;

    std::array<UnorderedCheckSum, kProducers> produced;
    std::atomic<uint64_t> produced_cnt{0};
    std::atomic<size_t> alive_producers{kProducers};
    for (size_t i = 0; i < kProducers; ++i) {
        r.Run(
            [seed = rng.Generate64(), &produced, &q, &produced_cnt, i,
             &alive_producers](auto should_run) {
                PCGRandom rng{seed};
                UnorderedCheckSum p;

                uint64_t cnt = 0;
                while (should_run()) {
                    auto value = rng.Generate64();
                    p.Append(value);
                    q.Push(value);
                    ++cnt;
                }
                if (alive_producers.fetch_sub(1) == 1) {
                    q.Close();
                }

                produced[i] = p;
                produced_cnt.fetch_add(cnt);
            },
            500ms);
    }

    std::array<UnorderedCheckSum, kConsumers> consumed;
    std::atomic<uint64_t> consumed_cnt{0};
    for (size_t i = 0; i < kConsumers; ++i) {
        r.Run(
            [seed = rng.Generate64(), &consumed, &q, &consumed_cnt,
             i](auto should_run) {
                PCGRandom rng{seed};
                UnorderedCheckSum c;
                uint64_t cnt = 0;

                while (auto value = q.Pop()) {
                    ++cnt;
                    c.Append(*value);
                }

                consumed[i] = c;
                consumed_cnt.fetch_add(cnt);

                while (should_run()) {
                }
            },
            500ms);
    }

    std::move(r).Join();

    REQUIRE(produced_cnt.load() == consumed_cnt.load());
    if (kBuildType == BuildType::Release) {
        INFO("Inefficient");
        REQUIRE(produced_cnt.load() > 300'000);
    }

    UnorderedCheckSum p;
    for (auto& x : produced) {
        p.Merge(x);
    }

    UnorderedCheckSum c;
    for (auto& x : consumed) {
        c.Merge(x);
    }

    REQUIRE(p.GetDigest() == c.GetDigest());
}
