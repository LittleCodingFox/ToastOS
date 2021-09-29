#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

long int strtol(const char* nptr, char** endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
//double strtod(const char* nptr, char** endptr);
void srand(unsigned int seed);
int rand();
char* itoa(int num, char* str, int base);
int atoi(const char* s);
int abs(int n);
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif

