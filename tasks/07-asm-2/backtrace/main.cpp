#include <stdio.h>

extern "C" void print_tb();

#define BODY                                                                   \
    do {                                                                       \
        if (seed == 1) {                                                       \
            print_tb();                                                        \
            return 0;                                                          \
        }                                                                      \
        if (seed % 2) {                                                        \
            return foo(3 * seed + 1);                                          \
        } else {                                                               \
            return bar(seed / 2);                                              \
        }                                                                      \
    } while (0)

extern "C" int foo(int seed);
extern "C" int bar(int seed);

int main() {
    int seed;
    if (scanf("%d", &seed) != 1) {
        return 1;
    }
    BODY;
}

extern "C" int bar(int seed) {
    BODY;
}

extern "C" int foo(int seed) {
    BODY;
}
