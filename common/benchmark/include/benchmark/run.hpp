#pragma once

#include <benchmark/compiler.hpp>
#include <benchmark/timer.hpp>

template <class F>
CPUTimer::Times Run(F&& f) {
    CPUTimer timer;
    DoNotReorder();
    DoNotOptimize(f());
    DoNotReorder();
    return timer.GetTimes();
}

template <class F>
CPUTimer::Times RunWithWarmup(F&& f, size_t warmup, size_t measures) {
    for (size_t i = 0; i < warmup; ++i) {
        DoNotOptimize(f());
    }
    DoNotReorder();

    CPUTimer timer;
    DoNotReorder();

    for (size_t i = 0; i < measures; ++i) {
        DoNotOptimize(f());
    }
    DoNotReorder();

    return timer.GetTimes();
}
