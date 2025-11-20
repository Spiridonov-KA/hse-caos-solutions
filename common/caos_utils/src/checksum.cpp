#include <checksum.hpp>

#include <bit>

// Some silly bijection
uint64_t Hash(uint64_t c) {
    c += 0x15373035281943C9;
    c ^= 0x895EBF833CC1E9FB;
    c *= 0x61FAB941BB1CF167;
    return c;
}

uint64_t OrderedCheckSum::GetDigest() const {
    return digest_;
}

void OrderedCheckSum::Append(char c) {
    Append(static_cast<uint64_t>(c));
}

void OrderedCheckSum::Append(uint64_t c) {
    digest_ *= 424243;
    digest_ ^= Hash(c);
    digest_ = std::rotr(digest_, digest_ >> 58);
}

void OrderedCheckSum::Append(std::span<const char> r) {
    for (auto c : r) {
        Append(c);
    }
}

void OrderedCheckSum::Reset() {
    *this = OrderedCheckSum();
}

uint64_t UnorderedCheckSum::GetDigest() const {
    return digest_;
}

void UnorderedCheckSum::Append(uint64_t v) {
    digest_ += Hash(v);
}

void UnorderedCheckSum::Merge(const UnorderedCheckSum& other) {
    digest_ += other.digest_;
}
