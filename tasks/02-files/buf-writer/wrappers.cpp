#include "wrappers.hpp"

#include <cerrno>
#include <cstdlib>
#include <sys/types.h>
#include <utility>

size_t WritesCount = 0;
int TargetFd = 0;
int NextError = 0;
bool EnableTrim = false;

extern "C" ssize_t __real_write(int fd, const char* buf, size_t cnt);

extern "C" ssize_t __wrap_write(int fd, const char* buf, size_t cnt) {
    if (fd == TargetFd) {
        ++WritesCount;
        if (auto next = std::exchange(NextError, 0)) {
            errno = next;
            return -1;
        }

        if (EnableTrim && cnt > 0) {
            cnt = rand() % cnt + 1;
        }
    }

    return __real_write(fd, buf, cnt);
}
