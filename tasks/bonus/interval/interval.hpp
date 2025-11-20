#pragma once

#include <cfenv>
#include <unused.hpp>  // TODO: remove before flight.

struct Interval {
    Interval(double value = 0) : lower_(value), upper_(value) {
    }

    Interval(double lower, double upper) : lower_(lower), upper_(upper) {
    }

    Interval operator+(const Interval& other) const {
        UNUSED(other);  // TODO: remove before flight.
        return *this;  // TODO: remove before flight.
    }

    Interval operator-() const {
        return *this;  // TODO: remove before flight.
    }

    Interval operator-(const Interval& other) const {
        UNUSED(other);  // TODO: remove before flight.
        return *this;  // TODO: remove before flight.
    }

    Interval operator*(const Interval& other) const {
        UNUSED(other);  // TODO: remove before flight.
        return *this;  // TODO: remove before flight.
    }

    Interval operator/(double denom) const {
        UNUSED(denom);  // TODO: remove before flight.
        return *this;  // TODO: remove before flight.
    }

    Interval& operator+=(const Interval& other) {
        return *this = *this + other;
    }

    Interval& operator-=(const Interval& other) {
        return *this = *this - other;
    }

    Interval& operator*=(const Interval& other) {
        return *this = *this * other;
    }

    Interval& operator/=(double denom) {
        return *this = *this / denom;
    }

    double GetLower() const {
        return lower_;
    }

    double GetUpper() const {
        return upper_;
    }

  private:

    double lower_;
    double upper_;
};
