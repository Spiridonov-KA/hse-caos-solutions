#pragma once

#include <benchmark/compiler.hpp>
#include <benchmark/timer.hpp>

template <class F>
void InvokeDoNotOptimize(F&& f) {
    if constexpr (std::is_same_v<std::invoke_result_t<F>, void>) {
        f();
    } else {
        DoNotOptimize(f());
    }
}

template <class F>
CPUTimer::Times Run(F&& f) {
    CPUTimer timer;
    InvokeDoNotOptimize(std::forward<F>(f));
    return timer.GetTimes();
}

template <class F>
CPUTimer::Times RunWithWarmup(F&& f, size_t warmup, size_t measures) {
    for (size_t i = 0; i < warmup; ++i) {
        InvokeDoNotOptimize(f);
    }

    CPUTimer timer;

    for (size_t i = 0; i < measures; ++i) {
        InvokeDoNotOptimize(f);
    }

    return timer.GetTimes();
}
