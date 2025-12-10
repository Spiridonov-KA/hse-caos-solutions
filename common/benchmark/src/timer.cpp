#include <benchmark/timer.hpp>

#include <benchmark/compiler.hpp>

#include <cstring>
#include <iostream>
#include <time.h>

namespace {

int ToClockType(CPUTimer::Type type) {
    switch (type) {
    case CPUTimer::Thread:
        return CLOCK_THREAD_CPUTIME_ID;
    case CPUTimer::Process:
        return CLOCK_PROCESS_CPUTIME_ID;
    default:
        std::terminate();
    }
}

std::chrono::nanoseconds ToDuration(timespec d) {
    return std::chrono::seconds{d.tv_sec} + std::chrono::nanoseconds{d.tv_nsec};
}

CPUTimer::Times GetTimes(CPUTimer::Type type) {
    timespec usage;
    DoNotReorder();
    if (::clock_gettime(ToClockType(type), &usage) < 0) {
        int err = errno;
        std::cerr << "Failed to get resource usage: " << strerror(err)
                  << std::endl;
        std::abort();
    }
    DoNotReorder();
    return CPUTimer::Times{
        .wall_time = CPUTimer::WallClock::now().time_since_epoch(),
        .cpu_time = ToDuration(usage),
    };
}

}  // namespace

CPUTimer::CPUTimer(Type type) : type_{type}, start_(::GetTimes(type)) {
}

CPUTimer::Times CPUTimer::GetTimes() const {
    return ::GetTimes(type_) - start_;
}
