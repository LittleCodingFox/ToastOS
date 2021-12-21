#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallTCBSet(InterruptStack *stack)
{
    uint64_t base = stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: tcbset base %p", base);
#endif

    ProcessInfo *process = globalProcessManager->CurrentProcess();

    process->fsBase = base;

    DEBUG_OUT("Setting process %llu fsbase to %p", process->ID, base);

    globalProcessManager->LoadFSBase(base);

    return 0;
}