#pragma once

#include <benchmark/compiler.hpp>
#include <benchmark/timer.hpp>

template <class F>
CPUTimer::Times Run(F&& f) {
    CPUTimer timer;
    DoNotOptimize(f());
    return timer.GetTimes();
}

template <class F>
CPUTimer::Times RunWithWarmup(F&& f, size_t warmup, size_t measures) {
    for (size_t i = 0; i < warmup; ++i) {
        DoNotOptimize(f());
    }

    CPUTimer timer;

    for (size_t i = 0; i < measures; ++i) {
        DoNotOptimize(f());
    }

    return timer.GetTimes();
}
