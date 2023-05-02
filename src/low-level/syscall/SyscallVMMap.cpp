#include <string.h>
#include "syscall.hpp"
#include "debug.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "registers/Registers.hpp"
#include "errno.h"
#include "Panic.hpp"
#include "kasan/kasan.hpp"

#define PROT_NONE       0x00
#define PROT_READ       0x01
#define PROT_WRITE      0x02
#define PROT_EXEC       0x04

#define MAP_PRIVATE     0x01
#define MAP_SHARED      0x02
#define MAP_FIXED       0x04
#define MAP_ANON        0x08
#define MAP_ANONYMOUS   0x08

struct VMMapBag
{
    void *hint;
    size_t size;
    int prot;
    int flags;
    int fd;
    off_t offset;
};

int64_t SyscallVMMap(InterruptStack *stack)
{
    VMMapBag *bag = (VMMapBag *)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: vmmap hint %p size %lu prot 0x%x flags 0x%x fd %i offset: %lu", bag->hint, bag->size, bag->prot, bag->flags, bag->fd, bag->offset);
#endif

    if(bag->size == 0)
    {
        return -EINVAL;
    }

    uint64_t pages = bag->size / 0x1000 + 1;

    PageTableManager userManager;
    userManager.p4 = (PageTable *)Registers::ReadCR3();

    uint64_t pagingFlags = PAGING_FLAG_PRESENT | PAGING_FLAG_USER_ACCESSIBLE;

    if(bag->prot & PROT_WRITE)
    {
        pagingFlags |= PAGING_FLAG_WRITABLE;
    }

    if((bag->prot & PROT_EXEC) == 0)
    {
        //pagingFlags |= PAGING_FLAG_NO_EXECUTE;
    }

    if(bag->flags & MAP_ANONYMOUS)
    {
        if(bag->fd != -1 || bag->offset != 0)
        {
            return -1;
        }

        if(bag->flags & MAP_FIXED)
        {
            vector<void *> mappedPages;

            //TODO: Don't force hint/Add VMM
            for(uint64_t i = 0; i < pages; i++)
            {
                auto target = globalAllocator.RequestPage();
                auto higher = (void *)TranslateToHighHalfMemoryAddress((uint64_t)target);

                userManager.MapMemory(higher, target,
                    PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

                userManager.MapMemory((void *)((uint64_t)bag->hint + i * 0x1000), target, pagingFlags);

                UnpoisonKasanShadow(higher, 0x1000);

                memset(higher, 0, 0x1000);

                mappedPages.push_back(target);
            }

            processManager->AddProcessVMMap(bag->hint, mappedPages);

            //DEBUG_OUT("Mapping %p-%p with paging flags 0x%x", hint, (uint64_t)hint + pages * 0x1000, pagingFlags);

            return (uint64_t)bag->hint;
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

                    UnpoisonKasanShadow(higher, 0x1000);

                    memset(higher, 0, 0x1000);
                }

                processManager->AddProcessVMMap(physical, physical, pages);

                //DEBUG_OUT("Allocating %p-%p with paging flags 0x%x", physical, (uint64_t)physical + pages * 0x1000, pagingFlags);

                return (uint64_t)physical;
            }

            DEBUG_OUT("Failed to map memory for size %lu", bag->size);

            return -ENOMEM;
        }
    }
    else if(bag->fd != -1)
    {
        auto process = processManager->CurrentProcess();

        if(process == NULL || process->isValid == false)
        {
            return -EINVAL;
        }

        auto fd = process->info->GetFD(bag->fd);

        if(fd == NULL || fd->isValid == false || fd->impl == NULL)
        {
            return -EBADF;
        }

        uint8_t *buffer = new uint8_t[pages * 0x1000];

        memset(buffer, 0, pages * 0x1000);

        int error = 0;

        fd->impl->Seek(bag->offset, SEEK_SET, &error);

        if(error != 0)
        {
            delete [] buffer;

            return -EBADF;
        }

        fd->impl->Read(buffer, pages * 0x1000, &error);

        if(error != 0)
        {
            delete [] buffer;

            return -EBADF;
        }

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

                UnpoisonKasanShadow(higher, 0x1000);

                memcpy(higher, buffer + i * 0x1000, 0x1000);
            }

            delete [] buffer;

            processManager->AddProcessVMMap(physical, physical, pages);

            //DEBUG_OUT("Allocating %p-%p with paging flags 0x%x", physical, (uint64_t)physical + pages * 0x1000, pagingFlags);

            return (uint64_t)physical;
        }

        delete [] buffer;

        return -ENOMEM;
    }

    return -EINVAL;
}