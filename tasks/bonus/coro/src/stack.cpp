#include <stack.hpp>
#include <unused.hpp>  // TODO: remove before flight.

#include <cassert>
#include <sys/mman.h>

constexpr size_t kPageSize = 4096;

Stack Stack::Allocate(size_t pages) {
    UNUSED(pages, kPageSize);  // TODO: remove before flight.
    return {};  // TODO: remove before flight.
    // TODO: your code here.
}

Stack::~Stack() {
    // TODO: your code here.
}
