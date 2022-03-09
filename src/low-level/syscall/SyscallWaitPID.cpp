#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"
#include "wait.h"

int64_t SyscallWaitPID(InterruptStack *stack)
{
    pid_t pid = (pid_t)stack->rsi;
    int *status = (int *)stack->rdx;
    int flags = (int)stack->rcx;
    pid_t *retpid = (pid_t *)stack->r8;

    (void)status;
    (void)flags;

#if KERNEL_DEBUG_SYSCALLS || 1
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

        for(;;)
        {
            auto children = processManager->GetChildProcesses(current->info->ID);

            if(children.size() == 0)
            {
                if((flags & WNOHANG) == WNOHANG)
                {
                    return -EINTR;
                }

                ProcessYield();

                continue;
            }

            for(auto &child : children)
            {
                if(child.info->state != PROCESS_STATE_DEAD)
                {
                    if((flags & WNOHANG) == WNOHANG)
                    {
                        return -EINTR;
                    }

                    ProcessYield();

                    break;
                }

                if(child.info->didWaitPID || child.info->ID != pid)
                {
                    continue;
                }

                child.info->didWaitPID = true;

                *retpid = child.info->ID;

                return 0;
            }
        }
    }
    else if(pid == -1)
    {
        for(;;)
        {
            auto children = processManager->GetChildProcesses(current->info->ID);

            if(children.size() == 0)
            {
                if((flags & WNOHANG) == WNOHANG)
                {
                    return -EINTR;
                }

                ProcessYield();

                continue;
            }

            for(auto &child : children)
            {
                if(child.info->state != PROCESS_STATE_DEAD)
                {
                    if((flags & WNOHANG) == WNOHANG)
                    {
                        return -EINTR;
                    }

                    ProcessYield();

                    break;
                }

                if(child.info->didWaitPID)
                {
                    continue;
                }

                child.info->didWaitPID = true;

                *retpid = child.info->ID;

                return 0;
            }
        }
    }
    else if(pid == 0)
    {
        for(;;)
        {
            auto children = processManager->GetChildProcesses(current->info->ID);

            if(children.size() == 0)
            {
                if((flags & WNOHANG) == WNOHANG)
                {
                    return -EINTR;
                }

                ProcessYield();

                continue;
            }

            auto child = children[children.size() - 1];

            if(child.info->state != PROCESS_STATE_DEAD)
            {
                if((flags & WNOHANG) == WNOHANG)
                {
                    return -EINTR;
                }

                ProcessYield();

                continue;
            }
            
            *retpid = child.info->ID;

            return 0;
        }
    }
    else if(pid > 0)
    {
        for(;;)
        {
            auto process = processManager->GetProcess(pid);

            if(process == NULL || process->isValid == false || process->info->state != PROCESS_STATE_DEAD)
            {
                if((flags & WNOHANG) == WNOHANG)
                {
                    return -EINTR;
                }

                ProcessYield();

                continue;
            }

            *retpid = pid;

            return 0;
        }
    }

    return -EINTR;
}
