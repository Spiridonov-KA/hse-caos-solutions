#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" void print_tb();

static uint64_t CanaryValue(uint64_t seed) {
    for (size_t i = 0; i < 3; ++i) {
        seed ^= 424243;
        seed *= 12345;
    }
    return seed;
}

#define BODY(n, self)                                                          \
    do {                                                                       \
        volatile uint64_t canaries[n];                                         \
        for (size_t i = 0; i < n; ++i) {                                       \
            canaries[i] = CanaryValue(i);                                      \
        }                                                                      \
        int retval;                                                            \
        if (seed == 1) {                                                       \
            print_tb();                                                        \
            retval = 0;                                                        \
        } else if (seed % 2 != 0) {                                            \
            retval = foo(3 * seed + 1, print);                                 \
        } else {                                                               \
            retval = bar(seed / 2, print);                                     \
        }                                                                      \
        for (size_t i = 0; i < n; ++i) {                                       \
            assert(canaries[i] == CanaryValue(i));                             \
        }                                                                      \
        if (print) {                                                           \
            printf("I am %s\n", self);                                         \
        }                                                                      \
        return retval;                                                         \
    } while (0)

extern "C" int foo(int seed, bool print);
extern "C" int bar(int seed, bool print);

int main() {
    int seed;
    if (scanf("%d", &seed) != 1) {
        return 1;
    }
    bool print = std::getenv("PRINT_TB") != nullptr;
    BODY(2, "main");
}

extern "C" int bar(int seed, bool print) {
    BODY(3, "bar");
}

extern "C" int foo(int seed, bool print) {
    BODY(5, "foo");
}
