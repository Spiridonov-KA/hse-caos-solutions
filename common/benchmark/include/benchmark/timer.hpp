#pragma once

#include <chrono>

class CPUTimer {
  public:
    using WallClock = std::chrono::steady_clock;

    enum Type {
        Thread,
        Process,
    };

    struct Times {
        WallClock::duration wall_time;
        std::chrono::microseconds cpu_utime;
        std::chrono::microseconds cpu_stime;

        std::chrono::microseconds TotalCpuTime() const {
            return cpu_utime + cpu_stime;
        }

        Times operator-(const Times& other) const {
            return {
                .wall_time = wall_time - other.wall_time,
                .cpu_utime = cpu_utime - other.cpu_utime,
                .cpu_stime = cpu_stime - other.cpu_stime,
            };
        }
    };

    explicit CPUTimer(Type type = Type::Process);

    Times GetTimes() const;

  private:
    const Type type_;
    const Times start_;
};
