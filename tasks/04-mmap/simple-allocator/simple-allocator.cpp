#include "simple-allocator.hpp"

#include <syscalls.hpp>

#include <cstddef>

struct AllocatorState {
    static constexpr size_t kBlockSize = 16;
    static constexpr size_t kPageSize = 1 << 12;

    void* Allocate16() {
        return nullptr; // TODO: remove before flight.
        // TODO: your code here.
        throw 'TODO';
    }

    void Deallocate(void* ptr) {
        (void)ptr; // TODO: remove before flight.
        // TODO: your code here.
    }

    // TODO: your code here.
};

static AllocatorState allocator_state_;

void* Allocate16() {
    return allocator_state_.Allocate16();
}

void Deallocate16(void* ptr) {
    allocator_state_.Deallocate(ptr);
}
