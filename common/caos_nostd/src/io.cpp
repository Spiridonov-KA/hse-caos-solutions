#include <assert.hpp>
#include <io.hpp>
#include <strings.hpp>
#include <syscalls.hpp>

int WriteAll(int fd, const char* message, size_t len) {
    auto remaining = len;
    while (remaining > 0) {
        auto ret = Write(fd, message, remaining);
        if (ret == -1) {
            return ret;
        }

        auto written = static_cast<size_t>(ret);
        ASSERT_NO_REPORT(written <= remaining);
        remaining -= written;
        message += written;
    }

    return static_cast<int>(len);
}

void Print(const char* message) {
    int written = WriteAll(1, message, StrLen(message));
    ASSERT_NO_REPORT(written != -1);
}
