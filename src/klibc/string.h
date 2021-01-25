#ifndef STRING_H
#define STRING_H

#include "include/kernel.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

void *memcpy(void *dst, const void *src, size_t size);
void *memset(void *ptr, int val, size_t size);
void *memmove(void *dst, const void *src, size_t size);
int memcmp(const void *src1, const void *src2, size_t size);

uint32_t strlen(const char* string);
size_t strnlen(const char* string, size_t max_len);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strdup(const char* s);
char* strndup(const char* s, size_t n);
char* strchr(const char* s, int c);
char* strchrnul(const char* s, int c);
char* strrchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
int strncasecmp(const char* s1, const char* s2, size_t n);
int strcasecmp(const char* s1, const char* s2);

#ifdef __cplusplus
}
#endif

#endif
