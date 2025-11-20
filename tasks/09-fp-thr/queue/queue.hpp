#pragma once

#include <optional>

#include <unused.hpp>  // TODO: remove before flight.
// TODO: your code here.

template <class T>
struct Queue {
    void Push(T value) {
        UNUSED(value);  // TODO: remove before flight.
    }

    std::optional<T> Pop() {
        return std::nullopt;  // TODO: remove before flight.
    }

    void Close() {
    }

  private:
    // TODO: your code here.
};
