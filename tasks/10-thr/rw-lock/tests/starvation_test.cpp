#include "../rw-lock.hpp"
#include "common.hpp"

#include <thread>

using namespace std::chrono_literals;

TEST_CASE("NoWritersStarvation") {
    RWLock m;
    CriticalSectionState state;

    std::atomic<size_t> writers_finished = 0;

    std::vector<std::jthread> r_th;
    std::vector<std::thread> w_th;

    for (size_t i = 0; i < kReaders; ++i) {
        r_th.emplace_back([&](std::stop_token st) {
            while (!st.stop_requested()) {
                m.ReadLock();
                {
                    auto [_guard, _state] = state.Reader();
                    std::this_thread::sleep_for(100us);
                }
                m.ReadUnlock();
            }
        });
    }

    for (size_t i = 0; i < kWriters; ++i) {
        w_th.emplace_back([&]() {
            std::this_thread::sleep_for(10ms);
            for (int k = 0; k < 30; ++k) {
                m.WriteLock();
                {
                    auto [_guard, _state] = state.Writer();
                    std::this_thread::sleep_for(100us);
                }
                m.WriteUnlock();
            }
            if (writers_finished.fetch_add(1) + 1 == kWriters) {
                writers_finished.notify_one();
            }
        });
    }

    {
        size_t finished = 0;
        while ((finished = writers_finished.load()) != kWriters) {
            writers_finished.wait(finished);
        }
    }

    for (auto& th : r_th) {
        th.request_stop();
        th.join();
    }
    for (auto& th : w_th) {
        th.join();
    }
}
