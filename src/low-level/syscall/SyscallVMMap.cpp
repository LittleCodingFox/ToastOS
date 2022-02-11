#include <string.h>
#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "paging/PageFrameAllocator.hpp"
#include "Panic.hpp"

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
    void *hint = (void *)stack->rsi;
    size_t size = (size_t)stack->rdx;
    int prot = (int)stack->rcx;
    int flags = (int)stack->r8;
    int fd = (int)stack->r9;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: vmmap hint %p size %llu prot 0x%x flags 0x%x fd %i", hint, size, prot, flags, fd);
#endif

    (void)fd;

    uint64_t pages = size / 0x1000 + (size % 0x1000 != 0 ? 1 : 0);

    PageTableManager userManager;
    userManager.p4 = (PageTable *)Registers::ReadCR3();

    uint64_t pagingFlags = PAGING_FLAG_PRESENT | PAGING_FLAG_USER_ACCESSIBLE;

    if(prot & PROT_WRITE)
    {
        pagingFlags |= PAGING_FLAG_WRITABLE;
    }

    if((prot & PROT_EXEC) == 0)
    {
        //pagingFlags |= PAGING_FLAG_NO_EXECUTE;
    }

    if(flags & MAP_ANONYMOUS)
    {
        if(flags & MAP_FIXED)
        {
            vector<void *> mappedPages;

            //TODO: Don't force hint/Add VMM
            for(uint64_t i = 0; i < pages; i++)
            {
                auto target = globalAllocator.RequestPage();
                auto higher = (void *)TranslateToHighHalfMemoryAddress((uint64_t)target);

                userManager.MapMemory(higher, target,
                    PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

                userManager.MapMemory((void *)((uint64_t)hint + i * 0x1000), target, pagingFlags);

                memset(higher, 0, 0x1000);

                mappedPages.push_back(target);
            }

            globalProcessManager->AddProcessVMMap(hint, mappedPages);

            //DEBUG_OUT("Mapping %p-%p with paging flags 0x%x", hint, (uint64_t)hint + pages * 0x1000, pagingFlags);

            return (uint64_t)hint;
        }
        else
        {
            void *physical = globalAllocator.RequestPages(pages);

            if(physical != 0)
            {
                for(uint64_t i = 0; i < pages; i++)
                {
                    auto target = (void *)((uint64_t)physical + i * 0x1000);
                    auto higher = (void *)TranslateToHighHalfMemoryAddress((uint64_t)target);

                    userManager.IdentityMap(target, pagingFlags);

                    userManager.MapMemory(higher, target,
                        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

                    memset(higher, 0, 0x1000);
                }

                globalProcessManager->AddProcessVMMap(physical, physical, pages);

                //DEBUG_OUT("Allocating %p-%p with paging flags 0x%x", physical, (uint64_t)physical + pages * 0x1000, pagingFlags);

                return (uint64_t)physical;
            }

            DEBUG_OUT("Failed to map memory for size %llu", size);
        }
    }

    return 0;
}