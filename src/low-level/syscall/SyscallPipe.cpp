#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "input/InputSystem.hpp"
#include "debug.hpp"
#include "errno.h"
#include "ctype.h"

extern bool InputEnabled;

int64_t SyscallPipe(InterruptStack *stack)
{
    int *fds = (int *)stack->rsi;
    int flags = stack->rdx;

    (void)flags;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: Pipe fds: %p flags: %x", fds, flags);
#endif

    auto current = globalProcessManager->CurrentProcess();

    if(current == NULL || current->isValid == false)
    {
        return EFAULT;
    }

    current->info->CreatePipe(fds);

    return 0;
}