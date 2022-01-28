#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"

int64_t SyscallWaitPID(InterruptStack *stack)
{
    pid_t pid = (pid_t)stack->rsi;
    int *status = (int *)stack->rdx;
    int flags = (int)stack->rcx;
    pid_t *retpid = (pid_t *)stack->r8;

    (void)status;
    (void)flags;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: waitpid pid: %i (%x) status: %p flags: %i retpid: %p", pid, pid, status, flags, retpid);
#endif

    auto current = globalProcessManager->CurrentProcess();

    if(current == NULL || current->isValid == false)
    {
        return 0;
    }

    if(pid == -1)
    {
        auto children = globalProcessManager->GetChildProcesses(current->info->ID);

        if(children.size() == 0)
        {
            return -EINTR;
        }

        *retpid = children[children.size() - 1].info->ID;
    }
    else if(pid == 0)
    {
        auto children = globalProcessManager->GetChildProcesses(current->info->ID);

        if(children.size() == 0)
        {
            return -EINTR;
        }

        *retpid = children[children.size() - 1].info->ID;
    }
    else if(pid > 0)
    {
        auto process = globalProcessManager->GetProcess(pid);

        if(process == NULL || process->isValid == false)
        {
            return -EINTR;
        }

        *retpid = pid;
    }

    return 0;
}
