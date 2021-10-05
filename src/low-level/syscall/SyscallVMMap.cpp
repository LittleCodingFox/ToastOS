#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"

int64_t SyscallVMMap(InterruptStack *stack)
{
    ProcessInfo *process = globalProcessManager->CurrentProcess();

    void *hint = (void *)stack->rsi;
    size_t size = (size_t)stack->rdx;
    int prot = (int)stack->rcx;
    int flags = (int)stack->r8;
    int fd = (int)stack->r9;

    uint64_t ptr = (uint64_t)new uint8_t[size];

    if(ptr != 0)
    {
        PageTableManager userManager;
        userManager.p4 = (PageTable *)process->cr3;

        if(hint != NULL)
        {
            userManager.MapMemory(hint, (void *)TranslateToPhysicalMemoryAddress(ptr),
                PAGING_FLAG_PRESENT | PAGING_FLAG_USER_ACCESSIBLE | PAGING_FLAG_WRITABLE);

            return (int64_t)hint;
        }
        else
        {
            userManager.IdentityMap((void *)TranslateToPhysicalMemoryAddress(ptr), PAGING_FLAG_PRESENT | PAGING_FLAG_USER_ACCESSIBLE | PAGING_FLAG_WRITABLE);

            return TranslateToPhysicalMemoryAddress(ptr);
        }
    }

    return 0;
}