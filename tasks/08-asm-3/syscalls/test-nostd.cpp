#include "syscalls.hpp"

#include <macros.hpp>

#include <cerrno>
#include <cstdint>
#include <sys/mman.h>

asm(R"(
.global _start
_start:
    call Main

    mov rdi, rax
    mov rax, 60
    syscall
    ud2

.global memcpy
memcpy:
    test rdx, rdx
    mov rax, rdi

    jz .memcpy.end
.memcpy.loop:
    mov cl, [rsi]
    mov [rdi], cl
    inc rdi
    inc rsi
    dec rdx
    jnz .memcpy.loop
.memcpy.end:
    ret
)");

#define BIND(expr)                                                             \
    do {                                                                       \
        if (auto value = expr; value != 0) {                                   \
            return value;                                                      \
        }                                                                      \
    } while (false)

#define ASSERT(expr)                                                           \
    do {                                                                       \
        if (!(expr)) [[unlikely]] {                                            \
            const char msg[] = "Failed assertion " #expr " at " __FILE__       \
                               ":" STRINGIFY(__LINE__) "\n";                   \
            ::Write(2, msg, sizeof(msg));                                      \
            return 1;                                                          \
        }                                                                      \
    } while (false)

static int CheckPipe() {
    int fds[2];
    ASSERT(Pipe2(fds, 0) == 0);

#define EXPECTED "Hello, nostd!"
    char buf[] = EXPECTED;
    ASSERT(Write(fds[1], buf, sizeof(buf)) == sizeof(buf));

    char buf2[sizeof(buf)];
    ASSERT(Read(fds[0], buf2, sizeof(buf2)) == sizeof(buf2));

    char expected[] = EXPECTED;
#undef EXPECTED

    for (size_t i = 0; i < sizeof(buf); ++i) {
        ASSERT(expected[i] == buf2[i]);
    }

    ASSERT(Close(fds[0]) == 0);
    ASSERT(Close(fds[1]) == 0);
    ASSERT(Close(fds[0]) == -EBADF);
    ASSERT(Close(fds[1]) == -EBADF);

    return 0;
}

static int CheckMMap() {
    constexpr size_t kBlockSize = 4096;
    auto ptr = MMap(nullptr, kBlockSize, PROT_READ | PROT_WRITE,
                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    ASSERT(reinterpret_cast<uint64_t>(ptr) <= static_cast<uint64_t>(-4096));

    char* mem = reinterpret_cast<char*>(ptr);
    for (size_t i = 0; i < kBlockSize; ++i) {
        mem[i] = static_cast<char>(i);
    }

    ASSERT(MUnMap(mem, kBlockSize) == 0);

    return 0;
}

extern "C" int Main() {
    BIND(CheckPipe());
    BIND(CheckMMap());
    return 0;
}
