#pragma once

inline void Backoff() {
    asm volatile("pause");
}
