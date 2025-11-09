#pragma once

#include <cstdint>
#include <span>

struct CheckSum {
    uint64_t GetDigest() const;
    void Append(char c);
    void Append(std::span<const char> r);
    void Reset();

  private:
    uint64_t digest_ = 0;
};
