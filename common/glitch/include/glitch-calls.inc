#ifndef GLITCH_INTERNALS
#error "Don't include this header directly, use <glitch.hpp> instead"
#endif

XX(Main, main, int, int, argc, char**, argv, char**, env)

XX(Read, read, ssize_t, int, fd, void*, buf, size_t, count)
XX(ReadChk, __read_chk, ssize_t, int, fd, void*, buf, size_t, nbytes, size_t, buflen)
XX(PRead, pread, ssize_t, int, fd, void*, buf, size_t, count, off_t, offset)
XX(PReadChk, __pread_chk, ssize_t, int, fd, void*, buf, size_t, nbytes, off_t, offset, size_t, buflen)

XX(Write, write, ssize_t, int, fd, const void*, buf, size_t, count)
XX(PWrite, pwrite, ssize_t, int, fd, const void*, buf, size_t, count, off_t, offset)

XX(Fork, fork, pid_t)
