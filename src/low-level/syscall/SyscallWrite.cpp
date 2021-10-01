#include "syscall.hpp"
#include "interrupts/Interrupts.hpp"
#include "printf/printf.h"
#include "debug.hpp"

size_t SyscallWrite(InterruptStack *stack)
{
    uint64_t fd = stack->rsi;
    const void *buffer = (const void *)stack->rdx;
    size_t count = (size_t)stack->rcx;

    if(fd == 0) //stdin
    {
        //TODO
    }
    else if(fd == 1) //stdout
    {
        printf("%.*s", count, buffer);

        return count;
    }
    else if(fd == 2) //stderr
    {
        //TODO
    }
    else if(fd < 0)
    {
        return 0;
    }
    else if(fd > 2) //actual files
    {
        //TODO
    }

    return 0;
}