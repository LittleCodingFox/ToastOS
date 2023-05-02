#include <dirent.h>
#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"
#include "user/UserAccess.hpp"

int64_t SyscallReadEntries(InterruptStack *stack)
{
    int fd = stack->rsi;
    void *buffer = (void *)stack->rdx;
    uint64_t maxSize = stack->rcx;
    int *error = (int *)stack->r8;

    if(!SanitizeUserPointer(buffer) || !SanitizeUserPointer(error))
    {
        return -1;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: readentries fd: %i buffer %p maxSize %lu error %p", fd, buffer, maxSize, (void *)error);
#endif

    if(maxSize < sizeof(dirent))
    {
        *error = ENOENT;

        return -1;
    }

    auto current = processManager->CurrentProcess();

    if(current == NULL || current->isValid == false)
    {
        return -EBADF;
    }

    auto procfd = current->info->GetFD(fd);

    if(procfd == NULL)
    {
        return -EBADF;
    }

    auto entry = procfd->impl->ReadEntries();

    //End of directory
    if(entry == NULL)
    {
        *error = 0;

        return -1;
    }

    memcpy(buffer, entry, sizeof(dirent));

    return 0;
}