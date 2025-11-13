#include <benchmark/timer.hpp>

#include <benchmark/compiler.hpp>

#include <cstring>
#include <iostream>
#include <sys/resource.h>

namespace {

int ToRusageType(CPUTimer::Type type) {
    switch (type) {
    case CPUTimer::Thread:
        return RUSAGE_THREAD;
    case CPUTimer::Process:
        return RUSAGE_SELF;
    default:
        std::terminate();
    }
}

std::chrono::microseconds ToDuration(timeval d) {
    return std::chrono::microseconds{1'000'000ll * d.tv_sec + d.tv_usec};
}

CPUTimer::Times GetTimes(CPUTimer::Type type) {
    rusage usage;
    DoNotReorder();
    if (::getrusage(ToRusageType(type), &usage) < 0) {
        int err = errno;
        std::cerr << "Failed to get resource usage: " << strerror(err)
                  << std::endl;
        std::abort();
    }
    DoNotReorder();
    return CPUTimer::Times{
        .wall_time = CPUTimer::WallClock::now().time_since_epoch(),
        .cpu_utime = ToDuration(usage.ru_utime),
        .cpu_stime = ToDuration(usage.ru_stime),
    };
}

}  // namespace

CPUTimer::CPUTimer(Type type) : type_{type}, start_(::GetTimes(type)) {
}

CPUTimer::Times CPUTimer::GetTimes() const {
    return ::GetTimes(type_) - start_;
}
