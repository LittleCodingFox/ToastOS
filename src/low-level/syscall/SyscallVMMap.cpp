#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"

#define PROT_NONE       0x00
#define PROT_READ       0x01
#define PROT_WRITE      0x02
#define PROT_EXEC       0x04

#define MAP_PRIVATE     0x01
#define MAP_SHARED      0x02
#define MAP_FIXED       0x04
#define MAP_ANON        0x08
#define MAP_ANONYMOUS   0x08

int64_t SyscallVMMap(InterruptStack *stack)
{
    ProcessInfo *process = globalProcessManager->CurrentProcess();

    void *hint = (void *)stack->rsi;
    size_t size = (size_t)stack->rdx;
    int prot = (int)stack->rcx;
    int flags = (int)stack->r8;
    int fd = (int)stack->r9;

    DEBUG_OUT("VMMap: Hint: %p; size: %llu; prot: 0x%x; flags: 0x%x; fd: %i", hint, size, prot, flags, fd);

    uint64_t pages = size / 0x1000 + 1;

    PageTableManager userManager;
    userManager.p4 = (PageTable *)process->cr3;

    uint64_t pagingFlags = PAGING_FLAG_PRESENT | PAGING_FLAG_USER_ACCESSIBLE;

    if(prot & PROT_WRITE)
    {
        pagingFlags |= PAGING_FLAG_WRITABLE;
    }

    if((prot & PROT_EXEC) == 0)
    {
        pagingFlags |= PAGING_FLAG_NO_EXECUTE;
    }

    if(flags & MAP_ANONYMOUS)
    {
        if(flags & MAP_FIXED)
        {
            
        }
        else
        {
            uint64_t ptr = (uint64_t)new uint8_t[size];

            if(ptr != 0)
            {
                for(uint64_t i = 0; i < pages; i++)
                {
                    userManager.IdentityMap((void *)(TranslateToPhysicalMemoryAddress(ptr + i * 0x1000)), pagingFlags);
                }

                DEBUG_OUT("Identity Mapping %p-%p (%p-%p)", ptr, ptr + pages * 0x1000,
                    TranslateToPhysicalMemoryAddress(ptr), TranslateToPhysicalMemoryAddress(ptr + pages * 0x1000));

                return TranslateToPhysicalMemoryAddress(ptr);
            }

            DEBUG_OUT("Failed to identity map for size %llu", size);
        }
    }

    return 0;
}