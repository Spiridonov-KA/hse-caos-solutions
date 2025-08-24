#pragma once

#include <benchmark/compiler.hpp>
#include <benchmark/timer.hpp>

template <class F, class... Args>
void RunDontOptimize(F&& f, Args&&... args) {
    if constexpr (std::is_void_v<std::invoke_result_t<F>>) {
        f(std::forward<Args>(args)...);
    } else {
        DoNotOptimize(f(std::forward<Args>(args)...));
    }
}

template <class F>
CPUTimer::Times RunWithWarmup(F&& f, size_t warmup, size_t measured) {
    for (size_t i = 0; i < warmup; ++i) {
        RunDontOptimize(f);
    }
    DoNotReorder();

    CPUTimer timer;
    DoNotReorder();

    for (size_t i = 0; i < measured; ++i) {
        RunDontOptimize(f);
    }
    DoNotReorder();

    return timer.GetTimes();
}
