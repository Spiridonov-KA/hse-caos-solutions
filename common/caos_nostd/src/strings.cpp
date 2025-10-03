#include <strings.hpp>

size_t StrLen(const char* str) {
    size_t len = 0;
    while (*str != '\0') {
        ++str;
        ++len;
    }
    return len;
}
