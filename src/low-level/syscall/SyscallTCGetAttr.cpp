#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "signal.h"
#include "user/UserAccess.hpp"

int64_t SyscallTCGetAttr(InterruptStack *stack)
{
    int fd = stack->rsi;
    struct termios *attr = (struct termios *)stack->rdx;

    (void)fd;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: tcgetattr fd %i attr %p", fd, attr);
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
