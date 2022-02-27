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

    auto current = processManager->CurrentProcess();

    if(current == NULL || current->isValid == false)
    {
        return 0;
    }

    if(pid < -1)
    {
        pid = -pid;

        auto children = processManager->GetChildProcesses(current->info->ID);

        if(children.size() == 0)
        {
            return -EINTR;
        }

        for(auto &child : children)
        {
            if(child.info->state != PROCESS_STATE_DEAD)
            {
                return -EINTR;
            }

            if(child.info->didWaitPID || child.info->ID != pid)
            {
                continue;
            }

            child.info->didWaitPID = true;

            *retpid = child.info->ID;

            return 0;
        }

        return -EINTR;
    }
    else if(pid == -1)
    {
        auto children = processManager->GetChildProcesses(current->info->ID);

        if(children.size() == 0)
        {
            return -EINTR;
        }

        for(auto &child : children)
        {
            if(child.info->state != PROCESS_STATE_DEAD)
            {
                return -EINTR;
            }

            if(child.info->didWaitPID)
            {
                continue;
            }

            child.info->didWaitPID = true;

            *retpid = child.info->ID;

            return 0;
        }

        return -EINTR;
    }
    else if(pid == 0)
    {
        auto children = processManager->GetChildProcesses(current->info->ID);

        if(children.size() == 0)
        {
            return -EINTR;
        }

        auto child = children[children.size() - 1];

        if(child.info->state != PROCESS_STATE_DEAD)
        {
            return -EINTR;
        }
        
        *retpid = child.info->ID;

        return 0;
    }
    else if(pid > 0)
    {
        auto process = processManager->GetProcess(pid);

        if(process == NULL || process->isValid == false || process->info->state != PROCESS_STATE_DEAD)
        {
            return -EINTR;
        }

        *retpid = pid;

        return 0;
    }

    return 0;
}
