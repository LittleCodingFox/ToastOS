#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "debug.hpp"
#include "errno.h"

int64_t SyscallOpen(InterruptStack *stack)
{
    const char *path = (const char *)stack->rsi;
    uint32_t flags = (uint32_t)stack->rdx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: open path: %s flags: 0x%x", path, flags);
#endif

    auto currentProcess = processManager->CurrentProcess();

    if(currentProcess == NULL || currentProcess->isValid == false)
    {
        DEBUG_OUT("syscall_open: Current process is invalid", 0);

        return -ENOENT;
    }

    int error = 0;

    FILE_HANDLE handle = vfs->OpenFile(path, flags, currentProcess->info, &error);

    if(handle == INVALID_FILE_HANDLE)
    {
        DEBUG_OUT("syscall_open: Failed to open %s (flags: 0x%x)!", path, flags);

        return -ENOENT;
    }

    if(error != 0)
    {
        DEBUG_OUT("syscall_open: Failed to open %s (flags: 0x%x)!", path, flags);

        return -error;
    }

    auto fd = currentProcess->info->AddFD(PROCESS_FD_HANDLE, new ProcessFDVFS(handle));

    (void)fd;

    DEBUG_OUT("syscall_open: Opened %s with fd %i", path, fd);

    return fd;
}