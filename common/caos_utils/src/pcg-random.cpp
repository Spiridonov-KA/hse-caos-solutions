#include <pcg-random.hpp>

#include <bit>
#include <utility>

PCGRandom::PCGRandom(uint64_t seed, uint64_t inc)
    : state_{seed}, inc_{inc | 1} {
    Warmup();
}

uint32_t PCGRandom::operator()() {
    return Generate32();
}

uint32_t PCGRandom::Generate32() {
    uint64_t old_state =
        std::exchange(state_, state_ * 6364136223846793005ULL + inc_);
    uint32_t xorshifted =
        static_cast<uint32_t>(((old_state >> 18) ^ old_state) >> 27);
    int rot = old_state >> 59;
    return std::rotr(xorshifted, rot);
}

uint64_t PCGRandom::Generate64() {
    auto higher = Generate32();
    auto lower = Generate32();
    return (uint64_t{higher} << 32) | lower;
}

void PCGRandom::Warmup(size_t iterations) {
    for (size_t i = 0; i < iterations; ++i) {
        operator()();
    }
}
