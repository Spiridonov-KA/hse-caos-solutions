#include <benchmark/compiler.hpp>

#include <atomic>

namespace detail {

void DoNotOptimize(const void* ptr) {
    asm volatile("" ::"g"(ptr) : "memory");
}

}  // namespace detail

void DoNotReorder() {
    std::atomic_signal_fence(std::memory_order_seq_cst);
}
