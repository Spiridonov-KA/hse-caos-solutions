#include "syscall_impl.hpp"

#include <macros.hpp>
#include <syscalls.hpp>

#include <fcntl.h>
#include <sys/syscall.h>

int Errno = 0;

#define TO_SYSCALL_ARG(ty, name) ToSyscallArg(name)
#define ARG_DECL(ty, name) ty name

#define DEFINE_SYSCALL(ret, cpp_name, name, ...)                               \
    ret cpp_name(ARGS_MAP(ARG_DECL, __VA_ARGS__)) {                            \
        return FromSysret(                                                     \
            InternalSyscallImpl(SYS_##name,                                    \
                                ARGS_MAP(TO_SYSCALL_ARG, __VA_ARGS__)),        \
            To<ret>{});                                                        \
    }

// Everything inside this namespace has internal linkage so it's probably
// unnecessary
namespace {

template <class T>
int64_t ToSyscallArg(T* arg) {
    return reinterpret_cast<int64_t>(arg);
}

template <class T>
int64_t ToSyscallArg(T arg) {
    return static_cast<int64_t>(arg);
}

template <class T>
struct To {};

// https://elixir.bootlin.com/musl/v1.2.5/source/src/internal/syscall_ret.c#L4
template <class T>
T FromSysret(int64_t value, To<T>) {
    if (value > -4096 && value < 0) {
        Errno = static_cast<int>(-value);
        value = -1;
    }
    return static_cast<T>(value);
}

template <class T>
T* FromSysret(int64_t value, To<T*>) {
    return reinterpret_cast<T*>(FromSysret(value, To<int64_t>{}));
}

}  // namespace

// Arm doesn't provide 'open' syscall =(
int Open(const char* filename, int flags, mode_t mode) {
    return OpenAt(AT_FDCWD, filename, flags, mode);
}

DEFINE_SYSCALL(int, OpenAt, openat, int, dirfd, const char*, filename, int,
               flags, mode_t, mode)

DEFINE_SYSCALL(int, Close, close, int, fd)

void Exit(int status) {
    InternalSyscallImpl(SYS_exit, ToSyscallArg(status));
    __builtin_unreachable();
}

DEFINE_SYSCALL(ssize_t, Write, write, int, fd, const char*, buf, size_t, count)
DEFINE_SYSCALL(ssize_t, Read, read, int, fd, char*, buf, size_t, count)

DEFINE_SYSCALL(void*, MMap, mmap, void*, addr, size_t, length, int, prot, int,
               flags, int, fd, off_t, offset)
DEFINE_SYSCALL(void, MUnMap, munmap, void*, addr, size_t, length)
DEFINE_SYSCALL(void*, MReMap, mremap, void*, old_addr, void*, old_size, size_t,
               new_size, int, flags, void*, new_addr)

DEFINE_SYSCALL(int, MAdvise, madvise, void*, addr, size_t, length, int, advice)
