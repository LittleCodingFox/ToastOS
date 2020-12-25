#include "string.h"
#include <stddef.h>

void *memset(void *ptr, int val, size_t size) {

    void *temp = ptr;

    __asm__ volatile (

        "rep stosb"
        :"=D"(ptr),"=c"(size)
        :"0"(ptr),"a"(val),"1"(size)
        :"memory"
    );

    return temp;
}
