#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "kernel.h"
#include "user/UserAccess.hpp"

int64_t SyscallCWD(InterruptStack *stack)
{
    char *buffer = (char *)stack->rsi;
    size_t size = (size_t)stack->rdx;

    if(!SanitizeUserPointer(buffer))
    {
        return ENOENT;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: cwd buffer: %p size: %lu", buffer, size);
#endif

    auto process = processManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return ENOENT;
    }

    auto &cwd = process->info->cwd;

    if(cwd.size() + 1 > size)
    {
        return ERANGE;
    }

    memcpy(buffer, cwd.data(), cwd.size());

    buffer[size - 1] = '\0';

    return 0;
}
