#pragma once

#include <cstdint>

struct JumpBuf {
    // TODO: your code here.
};

extern "C" [[gnu::returns_twice]] uint64_t SetJump(JumpBuf* buf);
extern "C" [[noreturn]] void LongJump(JumpBuf* buf, uint64_t value);
