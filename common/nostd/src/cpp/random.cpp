#include <random.hpp>

#include <bit>
#include <utility>

PCGRandom::PCGRandom(uint64_t seed, uint64_t inc)
    : state_{seed}, inc_{inc | 1} {
}

PCGRandom::result_type PCGRandom::operator()() {
    uint64_t old_state =
        std::exchange(state_, state_ * 6364136223846793005ULL + inc_);
    uint32_t xorshifted =
        static_cast<uint32_t>(((old_state >> 18) ^ old_state) >> 27);
    int rot = old_state >> 59;
    return std::rotr(xorshifted, rot);
}

void PCGRandom::Warmup(size_t iterations) {
    for (size_t i = 0; i < iterations; ++i) {
        operator()();
    }
}
