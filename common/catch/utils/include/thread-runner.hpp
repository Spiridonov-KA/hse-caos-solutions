#pragma once

#include <chrono>
#include <thread>
#include <vector>

struct ThreadRunner {
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    template <class F>
    void Run(F&& f, Duration d) {
        running_.fetch_add(1);
        workers_.emplace_back([this, f = std::move(f), d]() {
            auto start = Clock::now();
            auto should_run = [start, d]() { return Clock::now() - start < d; };
            while (should_run()) {
                f(should_run);
            }
            running_.fetch_sub(1);
        });
    }

    void Join() && {
        for (auto& x : workers_) {
            x.join();
        }
    }

    size_t RunningWorkers() const {
        return running_.load();
    }

    size_t TotalWorkers() const {
        return workers_.size();
    }

  private:
    std::vector<std::jthread> workers_;
    std::atomic<size_t> running_{0};
};
