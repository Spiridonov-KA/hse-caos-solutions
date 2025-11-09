#include <checksum.hpp>

#include <bit>

uint64_t CheckSum::GetDigest() const {
    return digest_;
}

void CheckSum::Append(char c) {
    digest_ *= 424243;
    digest_ ^= static_cast<uint64_t>(c);
    digest_ = std::rotr(digest_, digest_ >> 58);
}

void CheckSum::Append(std::span<const char> r) {
    for (auto c : r) {
        Append(c);
    }
}

void CheckSum::Reset() {
    *this = CheckSum();
}
