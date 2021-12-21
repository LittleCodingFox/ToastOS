#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallSetUID(InterruptStack *stack)
{
    pid_t pid = stack->rsi;
    uid_t uid = (uid_t)stack->rdx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: SetUID pid: %i uid: %i", pid, uid);
#endif

    globalProcessManager->SetUID(pid, uid);

    return 0;
}