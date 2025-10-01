#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>

#include <unused.hpp>  // TODO: remove before flight.

struct RingBuffer {
    using ElementType = int64_t;
    static constexpr size_t kPageSize = 1 << 12;

    static std::variant<RingBuffer, int> Create(size_t capacity) {
        UNUSED(capacity);  // TODO: remove before flight.
        return 0;  // TODO: remove before flight.
        // TODO: your code here.
    }

    size_t Capacity() const {
        return 0;  // TODO: remove before flight.
        // TODO: your code here.
    }

    void PushBack(ElementType value) {
        UNUSED(value);  // TODO: remove before flight.
        // TODO: your code here.
    }

    void PopBack() {
        // TODO: your code here.
    }

    void PushFront(ElementType value) {
        UNUSED(value);  // TODO: remove before flight.
        // TODO: your code here.
    }

    void PopFront() {
        // TODO: your code here.
    }

    std::span<ElementType> Data() {
        return {};  // TODO: remove before flight.
        // TODO: your code here.
    }

  private:
    // TODO: your code here.
};
