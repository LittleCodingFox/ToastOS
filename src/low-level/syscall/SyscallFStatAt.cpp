#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"
#include "user/UserAccess.hpp"

int64_t SyscallFStatAt(InterruptStack *stack)
{
    struct stat *stat = (struct stat *)stack->rsi;
    int fd = (int)stack->rdx;
    const char *path = (const char *)stack->rcx;

    if(!SanitizeUserPointer(stat) || !SanitizeUserPointer(path))
    {
        return -EBADF;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: fstatat stat: %p; fd: %i; path %p", stat, fd, path);
#endif

    auto current = processManager->CurrentProcess();

    if(current == NULL || current->isValid == false)
    {
        return -EBADF;
    }

    string p = path;
    int error = 0;

    if((p.size() > 0 && p[0] == '/') || fd == AT_FDCWD)
    {
        FILE_HANDLE handle = vfs->OpenFile(path, O_RDONLY, current->info, &error);
        
        if(vfs->FileType(handle) == FILE_HANDLE_UNKNOWN || error != 0)
        {
            //DEBUG_OUT(error == 0 ? "Stat is unknown" : "Stat Error: %i", error);

            return -ENOENT;
        }

        *stat = vfs->Stat(handle, &error);

        vfs->CloseFile(handle);
    }
    else
    {
        auto procfd = current->info->GetFD(fd);

        if(procfd == NULL || procfd->isValid == false || procfd->type != ProcessFDType::Handle)
        {
            return -EBADF;
        }

        auto dir = (ProcessFDVFS *)procfd->impl;

        auto base = vfs->GetFilePath(dir->handle);

        if(base.size() == 0)
        {
            return -EBADF;
        }

        auto realPath = base + path;

        FILE_HANDLE handle = vfs->OpenFile(realPath.data(), O_RDONLY, current->info, &error);
        
        if(vfs->FileType(handle) == FILE_HANDLE_UNKNOWN || error != 0)
        {
            //DEBUG_OUT(error == 0 ? "Stat is unknown" : "Stat Error: %i", error);

            return -ENOENT;
        }

        *stat = vfs->Stat(handle, &error);

        vfs->CloseFile(handle);
    }

    if(error != 0)
    {
        return -error;
    }

    return 0;
}