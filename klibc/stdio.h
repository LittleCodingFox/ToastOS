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

#ifdef __cplusplus
}
#endif

#endif

