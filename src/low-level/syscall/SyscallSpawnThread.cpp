#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "user/UserAccess.hpp"

int64_t SyscallSpawnThread(InterruptStack *stack)
{
    uint64_t rip = stack->rsi;
    uint64_t rsp = stack->rdx;

    if(!SanitizeUserPointer((void *)rip) || !SanitizeUserPointer((void *)rsp))
    {
        return 0;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: spawn thread rip: %p rsp: %p", rip, rsp);
#endif

    auto process = processManager->CurrentProcess();
    
    if(process == NULL || process->isValid == false)
    {
        return 0;
    }

    auto pcb = processManager->AddThread(rip, rsp);

    if(pcb == NULL)
    {
        return -EAGAIN;
    }

    return pcb->tid;
}