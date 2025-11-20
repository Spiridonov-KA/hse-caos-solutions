#pragma once

#include "backoff.hpp"
#include "thread-runner.hpp"

#include <atomic>

struct StepThreadRunner : protected ThreadRunner {
    using ThreadRunner::Join;
    using ThreadRunner::RunningWorkers;
    using ThreadRunner::TotalWorkers;

    template <class F>
    void Add(F&& f, Duration d) {
        ThreadRunner::Run(
            [this, f = std::move(f)](auto should_run) {
                this->RunSteps(f, should_run);
                while (should_run()) {
                    Backoff();
                }
            },
            d);
    }

    bool DoStep() {
        step_finishes_.store(0);
        step_.store(step_.load() + 1);

        auto total_workers = TotalWorkers();
        while (step_finishes_.load() != total_workers) {
            Backoff();
        }
        return step_.load() < last_step_.load();
    }

  private:
    template <class F>
    void RunSteps(F&& f, auto should_run) {
        uint64_t cur_step = 1;
        bool finish = false;
        while (true) {
            while (step_.load() != cur_step) {
                if (!should_run()) [[unlikely]] {
                    ProposeLastStep(cur_step);
                }
                if (last_step_.load() < cur_step) [[unlikely]] {
                    finish = true;
                    break;
                }
                Backoff();
            }

            if (finish) [[unlikely]] {
                break;
            }

            f();

            step_finishes_.fetch_add(1);
            ++cur_step;
        }
    }

    uint64_t ProposeLastStep(uint64_t step) {
        uint64_t cur = last_step_.load();
        while (cur > step) {
            if (last_step_.compare_exchange_weak(cur, step)) {
                return step;
            }
        }
        return cur;
    }

    static constexpr uint64_t kDefaultLastStep =
        std::numeric_limits<uint64_t>::max();

    std::atomic<uint64_t> step_{0};
    std::atomic<uint64_t> step_finishes_{0};
    std::atomic<uint64_t> last_step_{kDefaultLastStep};
};
