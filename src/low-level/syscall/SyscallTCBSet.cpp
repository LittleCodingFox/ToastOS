#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallTCBSet(InterruptStack *stack)
{
    uint64_t base = stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: tcbset base %p", base);
#endif

    auto thread = processManager->CurrentThread();
    
    if(thread == NULL)
    {
        return 0;
    }

    thread->fsBase = base;

    DEBUG_OUT("Setting thread %i fsbase to %p", thread->tid, base);

    processManager->LoadFSBase(base);

    return 0;
}