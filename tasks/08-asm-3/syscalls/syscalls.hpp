#pragma once

#include <sys/types.h>

int Pipe2(int* pipes, int flags);
int Open(const char* filename, int flags, mode_t mode);
int Close(int fd);

ssize_t Write(int fd, const char* buf, size_t count);
ssize_t Read(int fd, char* buf, size_t count);

void* MMap(void* mem, size_t len, int prot, int flags, int fd, off_t offset);
int MUnMap(void* mem, size_t len);
