#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallKill(InterruptStack *stack)
{
    pid_t pid = stack->rsi;
    int signal = (int)stack->rdx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: kill pid: %i signal: %i", pid, signal);
#endif

    globalProcessManager->Kill(pid, signal);

    return 0;
}