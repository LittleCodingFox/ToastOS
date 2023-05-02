#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "random/random.h"
#include "user/UserAccess.hpp"

int64_t SyscallGetRandom(InterruptStack *stack)
{
    void *buffer = (void *)stack->rsi;
    size_t length = (size_t)stack->rdx;
    int flags = (int)stack->rcx;

    (void)flags;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: GetRandom buffer: %p length: %lu flags: %x", buffer, length, flags);
#endif

    if(SanitizeUserPointer(buffer) == false)
    {
        DEBUG_OUT("Buffer %p is invalid", buffer);

        return EFAULT;
    }

    uint8_t *outBuffer = new uint8_t[length];

    for(auto i = 0; i < length; i++)
    {
        outBuffer[i] = (uint8_t)random();
    }

    if(WriteUserArray<uint8_t>((uint8_t *)buffer, outBuffer, length) == false)
    {
        return EFAULT;
    }

    return length;
}