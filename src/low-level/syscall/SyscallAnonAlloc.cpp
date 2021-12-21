#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"

int64_t SyscallAnonAlloc(InterruptStack *stack)
{
    size_t size = (size_t)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: Anon-alloc size: %llu", size);
#endif

    uint64_t ptr = (uint64_t)new uint8_t[size];

    if(ptr != 0)
    {
        PageTableManager userManager;
        userManager.p4 = (PageTable *)Registers::ReadCR3();

        userManager.IdentityMap((void *)TranslateToPhysicalMemoryAddress(ptr), PAGING_FLAG_PRESENT | PAGING_FLAG_USER_ACCESSIBLE | PAGING_FLAG_WRITABLE);

        return TranslateToPhysicalMemoryAddress(ptr);
    }

    return 0;
}