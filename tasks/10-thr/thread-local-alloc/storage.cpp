#include "storage.hpp"
#include <unused.hpp>  // TODO: remove before flight.

void* Alloc(size_t size, size_t alignment) {
    UNUSED(size, alignment);  // TODO: remove before flight.
    return nullptr;  // TODO: remove before flight.
}

void Dealloc() {
    // TODO: your code here.
}

void OnThreadStart(Stat* stat) {
    UNUSED(stat);  // TODO: remove before flight.
}

void OnThreadStop() {
}
