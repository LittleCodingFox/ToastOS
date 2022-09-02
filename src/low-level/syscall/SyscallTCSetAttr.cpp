#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "signal.h"
#include "user/UserAccess.hpp"

int64_t SyscallTCSetAttr(InterruptStack *stack)
{
    int fd = stack->rsi;
    int optional_action = stack->rdx;
    const struct termios *attr = (const struct termios *)stack->rcx;

    (void)fd;
    (void)optional_action;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: tcsetattr fd %i optional_action %x attr %p", fd, optional_action, attr);
#endif

    if(!SanitizeUserPointer(attr))
    {
        return EINVAL;
    }

    auto process = processManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return 0;
    }

    return 0;
}
