#pragma once

#include <cstdint>
#ifdef USE_WRONG_1
extern "C" uint64_t Solve(uint64_t, uint64_t, uint64_t);
#else
extern "C" uint64_t Solve(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                          uint64_t, uint64_t);
#endif
