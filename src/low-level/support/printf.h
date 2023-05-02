#ifndef _PRINTF_H_
#define _PRINTF_H_

#include "nanoprintf/nanoprintf.h"

#define PRINTF_BUFFER_SIZE 4096

#if __cplusplus
extern "C" {
#endif

void LockPrintf();

void UnlockPrintf();

void PrintLine(const char *buffer);
void Print(const char *buffer);
char *PrintfBuffer();

#if __cplusplus
}
#endif

#define PRINTF(fn, format, ...) \
    { \
        LockPrintf(); \
        npf_snprintf(PrintfBuffer(), PRINTF_BUFFER_SIZE, format, __VA_ARGS__); \
        fn(PrintfBuffer()); \
        UnlockPrintf(); \
    }

#define SPRINTF(target, format, ...) \
    { \
        LockPrintf(); \
        npf_snprintf(PrintfBuffer(), PRINTF_BUFFER_SIZE, format, __VA_ARGS__); \
        memcpy(target, PrintfBuffer(), strlen(PrintfBuffer())); \
        target[strlen(PrintfBuffer())] = '\0'; \
        UnlockPrintf(); \
    }

#define SNPRINTF(target, size, format, ...) \
    { \
        LockPrintf(); \
        npf_snprintf(target, size, format, __VA_ARGS__); \
        UnlockPrintf(); \
    }

#define printf(format, ...) PRINTF(Print, format, __VA_ARGS__);
#define sprintf(target, format, ...) SPRINTF(target, format, __VA_ARGS__);
#define snprintf(target, size, format, ...) SNPRINTF(target, size, format, __VA_ARGS__);
#define vsnprintf(target, size, format, ...) npf_vsnprintf(target, size, format, __VA_ARGS__);

#define print(text) \
    { \
        LockPrintf(); \
        Print(text); \
        UnlockPrintf(); \
    }

#endif
