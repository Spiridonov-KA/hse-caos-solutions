#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

// https://www.pcg-random.org/pdf/hmc-cs-2014-0905.pdf
class PCGRandom {
  public:
    PCGRandom(uint64_t seed, uint64_t inc = 0);

    using result_type = uint32_t;

    result_type operator()();

    uint64_t Generate64();

    uint32_t Generate32();

    void Warmup(size_t iterations = 3);

    static constexpr result_type min() {
        return std::numeric_limits<result_type>::min();
    }

    static constexpr result_type max() {
        return std::numeric_limits<result_type>::max();
    }

  private:
    uint64_t state_;
    uint64_t inc_;
};
