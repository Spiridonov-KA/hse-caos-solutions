#pragma once

#include <cstddef>

struct Stat {
    size_t allocation_num{0};
    size_t bytes_allocated{0};

    void Record(size_t size) {
        ++allocation_num;
        bytes_allocated += size;
    }
};

// Will alloc no more than 1MB
void* Alloc(size_t size, size_t alignment = alignof(std::max_align_t));
void Dealloc();

void OnThreadStart(Stat* stat);
void OnThreadStop();
