#include <benchmark/compiler.hpp>

namespace detail {

void DoNotOptimize(const void* ptr) {
    asm volatile("" ::"g"(ptr));
}

}  // namespace detail

void DoNotReorder() {
    asm volatile("" ::: "memory");
}
