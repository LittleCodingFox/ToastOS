#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "user/UserAccess.hpp"

int64_t SyscallTCBSet(InterruptStack *stack)
{
    uint64_t base = stack->rsi;

    if(!SanitizeUserPointer((void *)base))
    {
        return 0;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: tcbset base %p", (void *)base);
#endif

    auto thread = processManager->CurrentThread();
    
    if(thread == NULL)
    {
        return 0;
    }

    thread->fsBase = base;

    DEBUG_OUT("Setting thread %i fsbase to %p", thread->tid, (void *)base);

    processManager->LoadFSBase(base);

    return 0;
}