#include "atomic.hpp"

#include <check-mt.hpp>
#include <pcg-random.hpp>
#include <step-thread-runner.hpp>
#include <thread-runner.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <numeric>

using namespace std::chrono_literals;

TEST_CASE("SingleThread") {
    AtomicU64 a{123};
    AtomicU64 b{456};

    a.Store(321);
    b.Store(654);
    REQUIRE(a.Load() == 321);
    REQUIRE(b.Load() == 654);

    {
        auto prev = a.Exchange(543);
        REQUIRE(prev == 321);
        REQUIRE(a.Load() == 543);
    }

    {
        auto prev = b.FetchAdd(543);
        REQUIRE(prev == 654);
        REQUIRE(b.Load() == 543 + 654);
    }
}

static uint64_t ReplicateByte(uint64_t v) {
    v |= (v << 8);
    v |= (v << 16);
    v |= (v << 32);
    return v;
};

TEST_CASE("StoreLoadConsistency") {
    AtomicU64 v(ReplicateByte(0));
    PCGRandom rng{Catch::getSeed()};

    ThreadRunner r;
    for (size_t i = 0; i < 3; ++i) {
        r.Run(
            [&v, seed = rng.Generate64()](auto) {
                PCGRandom rng{seed};
                auto cur = v.Load();

                CHECK_MT(ReplicateByte(cur & 0xFF) == cur);

                v.Store(ReplicateByte(rng.Generate32() & 0xFF));
            },
            100ms);
    }

    std::move(r).Join();
}

TEST_CASE("FetchAdd") {
    constexpr size_t kThreads = 3;

    PCGRandom rng{Catch::getSeed()};
    AtomicU64 v;

    ThreadRunner r;

    std::array<uint64_t, kThreads> totals{};

    for (size_t i = 0; i < kThreads; ++i) {
        r.Run(
            [&v, &totals, i, seed = rng.Generate64()](auto should_run) {
                PCGRandom rng{seed};
                uint64_t total = 0;

                while (should_run()) {
                    uint64_t delta = rng.Generate64();
                    total += delta;
                    v.FetchAdd(delta);
                }

                totals[i] = total;
            },
            100ms);
    }

    std::move(r).Join();

    auto total = std::accumulate(totals.begin(), totals.end(), uint64_t{0});

    REQUIRE(total == v.Load());
}

TEST_CASE("Exchange") {
    constexpr size_t kThreads = 3;

    PCGRandom rng{Catch::getSeed()};
    AtomicU64 v;

    ThreadRunner r;
    std::array<uint64_t, kThreads> total_written{};
    std::array<uint64_t, kThreads> total_read{};

    for (size_t i = 0; i < kThreads; ++i) {
        r.Run(
            [&v, &total_written, &total_read, i,
             seed = rng.Generate64()](auto should_run) {
                PCGRandom rng{seed};
                uint64_t written = 0;
                uint64_t read = 0;

                while (should_run()) {
                    auto new_value = rng.Generate64();
                    uint64_t old_value = v.Exchange(new_value);
                    read += old_value;
                    written += new_value;
                }

                total_written[i] = written;
                total_read[i] = read;
            },
            100ms);
    }

    std::move(r).Join();

    auto written = std::accumulate(total_written.begin(), total_written.end(),
                                   uint64_t{0});
    auto read =
        std::accumulate(total_read.begin(), total_read.end(), uint64_t{0});

    REQUIRE(written == read + v.Load());
}

TEST_CASE("StoreBuffering") {
    AtomicU64 a;
    AtomicU64 b;

    StepThreadRunner t;

    AtomicU64 r1;
    AtomicU64 r2;

    t.Add(
        [&]() {
            // SB test
            a.Store(1);
            r1.Store(b.Load());
        },
        500ms);

    t.Add(
        [&]() {
            // SB test
            b.Store(1);
            r2.Store(a.Load());
        },
        500ms);

    bool finish = false;
    while (!finish) {
        a.Store(0);
        b.Store(0);

        finish = !t.DoStep();

        REQUIRE((r1.Load() != 0 || r2.Load() != 0));
    }

    std::move(t).Join();
}
