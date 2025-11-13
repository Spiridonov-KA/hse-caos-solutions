#include <build.hpp>
#include <coroutine.hpp>

#include <benchmark/timer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <queue>

using namespace std::chrono_literals;

extern "C" uint64_t ReadRSP();
asm(R"(
ReadRSP:
    mov rax, rsp
    ret
)");

TEST_CASE("StackAlignment") {
    uint64_t rsp;
    auto c = Coroutine::Create([&rsp](Coroutine* self) {
        rsp = ReadRSP();
        self->Suspend();
        rsp = ReadRSP();
    });

    c->Resume();
    REQUIRE((rsp & 15) == 8);
    REQUIRE(!c->IsFinished());

    c->Resume();
    REQUIRE((rsp & 15) == 8);
    REQUIRE(c->IsFinished());
}

TEST_CASE("JustWorks") {
    std::vector<int> log;

    {
        auto c = Coroutine::Create([&log](Coroutine* self) {
            log.push_back(2);
            self->Suspend();
            log.push_back(4);
        });

        log.push_back(1);
        c->Resume();
        log.push_back(3);
        c->Resume();
        log.push_back(5);
    }

    REQUIRE(log == std::vector<int>{1, 2, 3, 4, 5});
}

TEST_CASE("RepeatedResumes") {
    auto c = Coroutine::Create([](Coroutine* self) {
        for (size_t i = 0; i < 20; ++i) {
            self->Suspend();
        }
    });

    size_t cnt = 0;
    while (!c->IsFinished()) {
        REQUIRE(cnt < 30);
        c->Resume();
        ++cnt;
    }

    REQUIRE(cnt == 21);
}

TEST_CASE("ManyCoroutines") {
    std::queue<std::unique_ptr<Coroutine>> tasks;
    std::vector<int> log;

    for (size_t i = 0; i < 1024; ++i) {
        tasks.emplace(Coroutine::Create([i, &log](Coroutine* self) {
            for (size_t j = 0; j < 16; ++j) {
                log.push_back(j * 1024 + i);
                self->Suspend();
            }
        }));
    }

    while (!tasks.empty()) {
        auto c = std::move(tasks.front());
        tasks.pop();
        c->Resume();
        if (!c->IsFinished()) {
            tasks.emplace(std::move(c));
        }
    }

    for (size_t i = 0; i < log.size(); ++i) {
        REQUIRE(log[i] == (int)i);
    }
}

TEST_CASE("NestedCoroutines") {
    auto body = [](auto body_fn, size_t i) -> std::function<void(Coroutine*)> {
        return [i, body_fn](Coroutine* self) {
            if (i == 0) {
                return;
            }
            auto c = Coroutine::Create(body_fn(body_fn, i - 1));
            while (!c->IsFinished()) {
                self->Suspend();
                c->Resume();
            }
        };
    };

    auto c = Coroutine::Create(body(body, 128));
    size_t cnt = 0;
    while (!c->IsFinished()) {
        c->Resume();
        ++cnt;
    }
    REQUIRE(cnt == 129);
}

TEST_CASE("ExhaustMemory") {
    for (size_t i = 0; i < 65536; ++i) {
        auto c = Coroutine::Create([](Coroutine*) {});
        REQUIRE(!c->IsFinished());
        c->Resume();
        REQUIRE(c->IsFinished());
    }
}

TEST_CASE("Performance") {
    if constexpr (kBuildType != BuildType::Release) {
        return;
    }
    CPUTimer timer;
    constexpr size_t kTasks = 512;
    constexpr size_t kIterations = 8192;

    std::vector<std::unique_ptr<Coroutine>> tasks;
    tasks.reserve(kTasks);
    for (size_t i = 0; i < kTasks; ++i) {
        tasks.emplace_back(Coroutine::Create([](Coroutine* self) {
            for (size_t i = 0; i < kIterations; ++i) {
                self->Suspend();
            }
        }));
    }

    for (size_t i = 0; i < kIterations + 1; ++i) {
        for (auto& c : tasks) {
            c->Resume();
        }
    }

    auto times = timer.GetTimes();
    REQUIRE(times.TotalCpuTime() < 400ms);
}
