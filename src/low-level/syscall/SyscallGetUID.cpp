#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallGetUID(InterruptStack *stack)
{
    pid_t pid = stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: GetUID pid: %i", pid);
#endif

    return globalProcessManager->GetUID(pid);
}