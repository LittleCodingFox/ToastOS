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

    DEBUG_OUT("VMMap: Hint: %p; size: %llu; prot: %i; flags: %i; fd: %i", hint, size, prot, flags, fd);

    uint64_t ptr = (uint64_t)new uint8_t[size];

    if(ptr != 0)
    {
        uint64_t pages = size / 0x1000 + 1;

        PageTableManager userManager;
        userManager.p4 = (PageTable *)process->cr3;

        if(hint != NULL)
        {
            for(uint64_t i = 0; i < pages; i++)
            {
                userManager.MapMemory((void *)((uint64_t)hint + i * 0x1000),
                    (void *)(TranslateToPhysicalMemoryAddress(ptr) + i * 0x1000),
                    PAGING_FLAG_PRESENT | PAGING_FLAG_USER_ACCESSIBLE | PAGING_FLAG_WRITABLE);
            }

            return (int64_t)hint;
        }
        else
        {
            for(uint64_t i = 0; i < pages; i++)
            {
                userManager.IdentityMap((void *)(TranslateToPhysicalMemoryAddress(ptr + i * 0x1000)),
                    PAGING_FLAG_PRESENT | PAGING_FLAG_USER_ACCESSIBLE | PAGING_FLAG_WRITABLE);
            }

            return TranslateToPhysicalMemoryAddress(ptr);
        }
    }

    return 0;
}