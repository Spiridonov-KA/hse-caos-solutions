#include "allocator.hpp"

#include <syscalls.hpp>

#include <type_traits>

#include <unused.hpp>  // TODO: remove before flight.

static constexpr size_t kPageSize = 1 << 12;

// TODO: your code here.

struct AllocatorState {

    void* Allocate(size_t size) {
        UNUSED(size, kPageSize);  // TODO: remove before flight.
        return nullptr;  // TODO: remove before flight.
    }

    void Deallocate(void* ptr) {
        UNUSED(ptr);  // TODO: remove before flight.
    }

    // TODO: your code here.
};

static_assert(std::is_trivially_constructible_v<AllocatorState>);
static AllocatorState allocator;

void* Allocate(size_t size) {
    return allocator.Allocate(size);
}

void Deallocate(void* ptr) {
    allocator.Deallocate(ptr);
}
