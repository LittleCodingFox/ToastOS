#include "string.h"
#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t size) {

    void *temp = dst;

    __asm__ volatile (
        
        "rep movsb"
        :"=D"(dst),"=S"(src),"=c"(size)
        :"0"(dst),"1"(src),"2"(size)
        :"memory"
    );

    return temp;
}
