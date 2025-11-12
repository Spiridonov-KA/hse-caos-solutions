#include "syscalls.hpp"

#include <sys/syscall.h>

#include <unused.hpp>  // TODO: remove before flight.

int Pipe2(int* pipes, int flags) {
    UNUSED(pipes, flags);  // TODO: remove before flight.
    return -1;  // TODO: remove before flight.
}

int Open(const char* filename, int flags, mode_t mode) {
    UNUSED(filename, flags, mode);  // TODO: remove before flight.
    return -1;  // TODO: remove before flight.
}

int Close(int fd) {
    UNUSED(fd);  // TODO: remove before flight.
    return -1;  // TODO: remove before flight.
}

ssize_t Write(int fd, const char* buf, size_t count) {
    UNUSED(fd, buf, count);  // TODO: remove before flight.
    return -1;  // TODO: remove before flight.
}

ssize_t Read(int fd, char* buf, size_t count) {
    UNUSED(fd, buf, count);  // TODO: remove before flight.
    return -1;  // TODO: remove before flight.
}

void* MMap(void* mem, size_t len, int prot, int flags, int fd, off_t offset) {
    UNUSED(mem, len, prot, flags, fd, offset);  // TODO: remove before flight.
    return nullptr;  // TODO: remove before flight.
}

int MUnMap(void* addr, size_t len) {
    UNUSED(addr, len);  // TODO: remove before flight.
    return -1;  // TODO: remove before flight.
}
