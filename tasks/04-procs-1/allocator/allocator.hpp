#pragma once

#include <cstddef>

void* Allocate(size_t size);
void Deallocate(void* ptr);
