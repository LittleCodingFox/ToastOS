#include "string.h"
#include <stddef.h>
#include <stdint.h>

void *memmove(void *dst, const void *src, size_t size) { 

    void *temp = dst;

    if((uint64_t)dst <= (uint64_t)src || (uint64_t)dst >= (uint64_t)src + size) {

        __asm__ volatile (

            "rep movsb"
            :"=D"(dst),"=S"(src),"=c"(size)
            :"0"(dst),"1"(src),"2"(size)
            :"memory"
        );

    } else {

        __asm__ volatile (

            "std\n\t"
            "rep movsb\n\t"
            "cld"
            :"=D"(dst), "=S"(src), "=c"(size)
            :"0"((uint64_t)dst + size - 1), "1"((uint64_t)src + size - 1), "2"(size)
            :"memory"
        );
    }

    return temp;
}
