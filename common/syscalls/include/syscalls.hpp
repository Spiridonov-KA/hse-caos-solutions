#pragma once

#include <sys/types.h>

extern int Errno;
extern char** Environ;

extern "C" [[noreturn]] void Unreachable();

int Open(const char* filename, int flags, mode_t mode);
int OpenAt(int dirfd, const char* filename, int flags, mode_t mode);

int Close(int fd);

[[noreturn]] void Exit(int status);

ssize_t Write(int fd, const char* buf, size_t count);
ssize_t Read(int fd, char* buf, size_t count);

void* MMap(void* addr, size_t length, int prot, int flags, int fd,
           off_t offset);
void MUnMap(void* addr, size_t length);
void* MReMap(void* old_addr, size_t old_size, size_t new_size, int flags,
             void* new_addr);

int MAdvise(void* addr, size_t length, int advice);
