#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int __real_sigaction(int signum, const struct sigaction* restrict act,
                     struct sigaction* restrict oldact);
sighandler_t __real_signal(int signum, sighandler_t handler);

static volatile sig_atomic_t do_interrupt_write;

static void interrupt_next_write(int sig) {
    do_interrupt_write = 1;
}

int __real_main(int argc, char** argv, char** envp);

// ----- Use alt stack for SIGTERM handler (ban stdio in handlers) -----

enum { STACK_DELTA = 64 * 1024 };

static stack_t sig_stack;

int __wrap_main(int argc, char** argv, char** envp) {
    // Setup stack
    sig_stack.ss_sp =
        mmap(NULL, MINSIGSTKSZ + STACK_DELTA, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (sig_stack.ss_sp == MAP_FAILED) {
        abort();
    }
    sig_stack.ss_size = MINSIGSTKSZ + STACK_DELTA;
    if (sigaltstack(&sig_stack, NULL) < 0) {
        abort();
    }
    // Interrupts output
    __real_sigaction(SIGRTMAX,
                     &(struct sigaction){.sa_handler = interrupt_next_write,
                                         .sa_flags = SA_RESTART},
                     NULL);
    return __real_main(argc, argv, envp);
}

int __wrap_sigaction(int signum, const struct sigaction* restrict act,
                     struct sigaction* restrict oldact) {
    if (signum != SIGTERM) {
        return __real_sigaction(signum, act, oldact);
    }
    struct sigaction newact = *act;
    newact.sa_flags |= SA_ONSTACK;
    if (newact.sa_flags & SA_RESTART) {
        // Disable interrupts
        __real_sigaction(SIGRTMAX, &(struct sigaction){.sa_handler = SIG_IGN},
                         NULL);
    }
    return __real_sigaction(signum, &newact, oldact);
}

sighandler_t __wrap_signal(int signum, sighandler_t handler) {
    if (signum != SIGTERM) {
        return __real_signal(signum, handler);
    }
    // Use BSD semantics.
    // Real signal() implementation may differ, but this should be ok for
    // SIGTERM.
    struct sigaction oldact;
    struct sigaction newact = {
        .sa_handler = handler,
        .sa_flags = SA_RESTART | SA_ONSTACK,
    };
    if (sigaction(signum, &newact, &oldact) < 0) {
        return SIG_ERR;
    }
    return oldact.sa_handler;
}

static void assert_not_handler() {
    int foo = 0;
    if ((uintptr_t)&foo - (uintptr_t)sig_stack.ss_sp < sig_stack.ss_size) {
        const char* err = "stdio function was called from a signal handler\n";
        if (write(STDERR_FILENO, err, strlen(err))) {
        }
        abort();
    }
}

int __real_fputc(int c, FILE* stream);

int __wrap_fputc(int c, FILE* stream) {
    assert_not_handler();
    if (do_interrupt_write) {
        do_interrupt_write = 0;
        errno = EINTR;
        return EOF;
    }
    return __real_fputc(c, stream);
}

int __real_fputs(const char* restrict s, FILE* restrict stream);

int __wrap_fputs(const char* restrict s, FILE* restrict stream) {
    assert_not_handler();
    if (do_interrupt_write) {
        do_interrupt_write = 0;
        errno = EINTR;
        return EOF;
    }
    return __real_fputs(s, stream);
}

int __real_putc(int c, FILE* stream);

int __wrap_putc(int c, FILE* stream) {
    assert_not_handler();
    if (do_interrupt_write) {
        do_interrupt_write = 0;
        errno = EINTR;
        return EOF;
    }
    return __real_putc(c, stream);
}

int __real_putchar(int c);

int __wrap_putchar(int c) {
    assert_not_handler();
    if (do_interrupt_write) {
        do_interrupt_write = 0;
        errno = EINTR;
        return EOF;
    }
    return __real_putchar(c);
}

int __real_puts(const char* s);

int __wrap_puts(const char* s) {
    assert_not_handler();
    if (do_interrupt_write) {
        do_interrupt_write = 0;
        errno = EINTR;
        return EOF;
    }
    return __real_puts(s);
}

size_t __real_fwrite(const void* restrict ptr, size_t size, size_t nmemb,
                     FILE* restrict stream);

size_t __wrap_fwrite(const void* restrict ptr, size_t size, size_t nmemb,
                     FILE* restrict stream) {
    assert_not_handler();
    if (do_interrupt_write) {
        do_interrupt_write = 0;
        errno = EINTR;
        return 0;
    }
    return __real_fwrite(ptr, size, nmemb, stream);
}

int __real_dprintf(int fd, const char* restrict format, ...);

int __wrap_dprintf(int fd, const char* restrict format, ...) {
    assert_not_handler();
    if (do_interrupt_write) {
        do_interrupt_write = 0;
        errno = EINTR;
        return EOF;
    }
    va_list args;
    va_start(args, format);
    int res = vdprintf(fd, format, args);
    va_end(args);
    return res;
}

int __real_fprintf(FILE* restrict stream, const char* restrict format, ...);

int __wrap_fprintf(FILE* restrict stream, const char* restrict format, ...) {
    assert_not_handler();
    if (do_interrupt_write) {
        do_interrupt_write = 0;
        errno = EINTR;
        return EOF;
    }
    va_list args;
    va_start(args, format);
    int res = vfprintf(stream, format, args);
    va_end(args);
    return res;
}

int __real_printf(const char* restrict format, ...);

int __wrap_printf(const char* restrict format, ...) {
    assert_not_handler();
    if (do_interrupt_write) {
        do_interrupt_write = 0;
        errno = EINTR;
        return EOF;
    }
    va_list args;
    va_start(args, format);
    int res = vprintf(format, args);
    va_end(args);
    return res;
}

int __real_fflush(FILE* stream);

int __wrap_fflush(FILE* stream) {
    assert_not_handler();
    return __real_fflush(stream);
}

ssize_t __real_write(int fd, const void* buf, size_t count);

ssize_t __wrap_write(int fd, const void* buf, size_t count) {
    if (do_interrupt_write) {
        do_interrupt_write = 0;
        errno = EINTR;
        return -1;
    }
    return __real_write(fd, buf, count);
}
