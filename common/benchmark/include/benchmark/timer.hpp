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
        WallClock::duration wall_time{0};
        std::chrono::nanoseconds cpu_time{0};

        Times operator+(const Times& other) const {
            return {
                .wall_time = wall_time + other.wall_time,
                .cpu_time = cpu_time + other.cpu_time,
            };
        }

        Times& operator+=(const Times& other) {
            return (*this) = (*this) + other;
        }

        Times operator-() const {
            return {
                .wall_time = -wall_time,
                .cpu_time = -cpu_time,
            };
        }

        Times& operator-=(const Times& other) {
            return (*this) = (*this) - other;
        }

        Times operator-(const Times& other) const {
            return (*this) + (-other);
        }
    };

    explicit CPUTimer(Type type = Type::Process);

    Times GetTimes() const;

  private:
    const Type type_;
    const Times start_;
};
