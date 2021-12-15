#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "signal.h"

int64_t SyscallSigprocMask(InterruptStack *stack)
{
    int how = stack->rsi;
    const sigset_t *set = (const sigset_t *)stack->rdx;
    sigset_t *retrieve = (sigset_t *)stack->rcx;

    ProcessInfo *process = globalProcessManager->CurrentProcess();

    sigset_t sigprocmask = process->sigprocmask;

    if(set == NULL)
    {
        *retrieve = sigprocmask;

        return 0;
    }

    switch(how)
    {
        case SIG_BLOCK:

            for(uint32_t i = 0; i < SIGNAL_MAX; i++)
            {
                if((*set & (sigset_t(1) << i)) == 0)
                {
                    sigprocmask |= sigset_t(1) << i;
                }
            }

            process->sigprocmask = sigprocmask;

            break;

        case SIG_UNBLOCK:

            for(uint32_t i = 0; i < SIGNAL_MAX; i++)
            {
                if((*set & (sigset_t(1) << i)) == 0)
                {
                    sigprocmask &= ~(sigset_t(1) << i);
                }
            }

            process->sigprocmask = sigprocmask;

            break;

        case SIG_SETMASK:

            process->sigprocmask = *set;

            break;

        default:

            return EINVAL;
    }

    return 0;
}