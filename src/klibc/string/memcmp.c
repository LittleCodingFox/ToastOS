#include "string.h"
#include <stddef.h>
#include <stdint.h>

int memcmp(const void *src1, const void *src2, size_t size) {

    if(!size) {

        return 0;
    }

    __asm__ volatile (

        "repe cmpsb"
        :"=D"(src1), "=S"(src2), "=c"(size)
        :"0"(src1),"1"(src2),"2"(size)
        :"memory","cc"
    );

    return ((const uint8_t *)src1)[-1] - ((const uint8_t *)src2)[-1];
}
