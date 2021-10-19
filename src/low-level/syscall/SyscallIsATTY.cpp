#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"

int64_t SyscallIsATTY(InterruptStack *stack)
{
    int fd = (int)stack->rsi;

    if(fd == 1)
    {
        return 1;
    }

    return 0;
}