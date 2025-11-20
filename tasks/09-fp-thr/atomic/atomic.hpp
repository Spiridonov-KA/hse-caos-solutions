#pragma once

#include <cstdint>

extern "C" uint64_t AtomicLoad(const uint64_t* ptr);
extern "C" void AtomicStore(uint64_t* ptr, uint64_t value);
extern "C" uint64_t AtomicExchange(uint64_t* ptr, uint64_t value);
extern "C" uint64_t AtomicFetchAdd(uint64_t* ptr, uint64_t delta);

struct AtomicU64 {
    AtomicU64(uint64_t initial = 0) : value_(initial) {
    }

    uint64_t Load() const {
        return AtomicLoad(&value_);
    }

    void Store(uint64_t v) {
        AtomicStore(&value_, v);
    }

    uint64_t Exchange(uint64_t v) {
        return AtomicExchange(&value_, v);
    }

    uint64_t FetchAdd(uint64_t delta) {
        return AtomicFetchAdd(&value_, delta);
    }

  private:
    uint64_t value_;
};
