#pragma once

#include <cstdint>
#include <span>

uint64_t Hash(uint64_t v);

struct OrderedCheckSum {
    uint64_t GetDigest() const;
    void Append(char c);
    void Append(uint64_t v);
    void Append(std::span<const char> r);
    void Reset();

  private:
    uint64_t digest_ = 0;
};

struct UnorderedCheckSum {
    uint64_t GetDigest() const;
    void Append(uint64_t v);

  private:
    uint64_t digest_ = 0;
};
