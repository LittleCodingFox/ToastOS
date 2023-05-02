#include <string.h>
#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "paging/PageFrameAllocator.hpp"
#include "user/UserAccess.hpp"

int64_t SyscallVMUnmap(InterruptStack *stack)
{
    void *ptr = (void *)stack->rsi;
    size_t size = (size_t)stack->rdx;

    (void)ptr;
    (void)size;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: vmunmap pointer %p size %lu", ptr, size);
#endif

    if(!SanitizeUserPointer(ptr))
    {
        return 0;
    }

    processManager->ClearProcessVMMap(ptr, size / 0x1000 + (size % 0x1000 != 0 ? 1 : 0));

    return 0;
}