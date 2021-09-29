#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern int stdin;
extern int stdout;
extern int stderr;

size_t write(int fd, const void *buffer, size_t count);

#ifdef __cplusplus
}
#endif

#endif

