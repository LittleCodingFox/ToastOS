#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "signal.h"
#include "user/UserAccess.hpp"

int64_t SyscallReadLink(InterruptStack *stack)
{
    const char *path = (const char *)stack->rsi;
    void *buffer = (void *)stack->rdx;
    size_t max_size = (size_t)stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: readlink path: %s buffer: %p max_size: %lu", path, buffer, max_size);
#endif

    if(!SanitizeUserPointer(path) || !SanitizeUserPointer(buffer))
    {
        return -EINVAL;
    }

    auto process = processManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return -ENOLINK;
    }

    string link;

    if(vfs->ReadLink(path, link, process->info) == false)
    {
        return -ENOLINK;
    }

    size_t length = link.size() >= max_size ? max_size - 1 : link.size() - 1;

    memcpy(buffer, link.data(), length);

    ((uint8_t *)buffer)[length] = '\0';

    return length;
}
