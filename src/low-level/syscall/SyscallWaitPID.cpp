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

    //DEBUG_OUT("Wait PID: %llu (%p); stats: %p; flags: %i; retpid: %p", pid, pid, status, flags, retpid);

    auto current = globalProcessManager->CurrentProcess();

    if(pid == (pid_t)-1)
    {
        auto children = globalProcessManager->GetChildProcesses(current->ID);

        if(children.size() == 0)
        {
            return -EINTR;
        }

        *retpid = children[0]->ID;

        return 0;
    }
    else if(pid == 0)
    {
        auto children = globalProcessManager->GetChildProcesses(current->ID);

        if(children.size() == 0)
        {
            return -EINTR;
        }

        *retpid = children[0]->ID;

        return 0;
    }
    else if(pid > 0)
    {
        ProcessInfo *process = globalProcessManager->GetProcess(pid);

        if(process == NULL)
        {
            return -EINTR;
        }

        *retpid = pid;
    }

    return 0;
}
