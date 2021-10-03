#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if !IS_LIBK
#include "printf/printf.h"
#endif

extern int stdin;
extern int stdout;
extern int stderr;

size_t write(int fd, const void *buffer, size_t count);
size_t read(int fd, const void *buffer, size_t count);
uint64_t seek(int fd, uint64_t offset, int whence);
int open(const char *path, uint32_t perms);
int close(int fd);

#ifdef __cplusplus
}
#endif

#endif

