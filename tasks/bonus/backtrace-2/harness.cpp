#include "src/backtrace.hpp"

#include <pcg-random.hpp>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

void Write(uint64_t* ptr, uint64_t value) {
    asm volatile("mov [%0], %1" ::"r"(ptr), "r"(value) : "memory");
}

extern "C" uint64_t Read(uint64_t* ptr) {
    uint64_t res;
    asm volatile(R"(
        mov %0, [%1]
    )"
                 : "=r"(res)
                 : "r"(ptr));
    return res;
}

PCGRandom rng{424243};

static void DoBadStuff() {
    uint64_t* ptr = reinterpret_cast<uint64_t*>(rng() & 4080);
    if (rng() & 1) {
        Write(ptr, rng.Generate64());
    } else {
        Read(ptr);
    }
    std::cout << "Accessed " << ptr << std::endl;
}

static void f(uint64_t n);
static void g(uint64_t n);
static void h(uint64_t n);
static void j(uint64_t n);
static void dummy(uint64_t);

#define BODY(n, m)                                                             \
    volatile uint64_t canary[m];                                               \
    uint64_t sum = 0;                                                          \
    for (auto& x : canary) {                                                   \
        x = rng.Generate64();                                                  \
        sum += x;                                                              \
    }                                                                          \
    if (n < 4) {                                                               \
        DoBadStuff();                                                          \
    } else {                                                                   \
        auto nn = n >> 2;                                                      \
        auto rem = n & 3;                                                      \
        if (rem == 0) {                                                        \
            f(nn);                                                             \
        } else if (rem == 1) {                                                 \
            g(nn);                                                             \
        } else if (rem == 2) {                                                 \
            h(nn);                                                             \
        } else if (rem == 3) {                                                 \
            j(nn);                                                             \
        } else {                                                               \
            dummy(nn);                                                         \
        }                                                                      \
    }                                                                          \
    for (auto& x : canary) {                                                   \
        sum -= x;                                                              \
    }                                                                          \
    if (sum != 0) {                                                            \
        std::cerr << "Checksum mismatch" << std::endl;                         \
        std::abort();                                                          \
    }

static __attribute__((noinline)) void f(uint64_t n) {
    BODY(n, 3);
}

static __attribute__((noinline)) void g(uint64_t n) {
    BODY(n, 4);
}

static __attribute__((noinline)) void h(uint64_t n) {
    BODY(n, 2);
}

static __attribute__((noinline)) void j(uint64_t n) {
    BODY(n, 5);
}

static void dummy(uint64_t) {
    std::abort();
}

int main() {
    uint64_t n;
    std::cin >> n;

    std::vector<Function> fns;

#define ADD(f, nf, p2)                                                         \
    do {                                                                       \
        if (n & p2) {                                                          \
            fns.push_back(Function{                                            \
                .name = #f,                                                    \
                .begin = reinterpret_cast<void*>(&f),                          \
                .end = reinterpret_cast<void*>(&nf),                           \
            });                                                                \
        }                                                                      \
    } while (false);

    ADD(f, g, 1);
    ADD(g, h, 2);
    ADD(h, j, 4);
    ADD(j, dummy, 8);

    // These are internal asserts
    for (auto& f : fns) {
        assert(f.begin < f.end);
    }
    for (size_t i = 0; i + 1 < fns.size(); ++i) {
        assert(fns[i].end <= fns[i + 1].begin);
    }

    SetupHandler(fns);

    uint64_t m;
    while (std::cin >> m) {
        f(m);
    }
}
