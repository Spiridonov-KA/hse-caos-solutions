#include "mul8.hpp"

#include <cstdio>
#include <cstdlib>

int main() {
    char* line = NULL;
    size_t sz = 0;
    float f;
    while (getline(&line, &sz, stdin) > 0) {
        char* eptr;
        f = strtof(line, &eptr);
        if (eptr == line) {  // non-float line
            continue;
        }
        SatMul8(&f);
        printf("%a\n", f);
    }
    free(line);
}
