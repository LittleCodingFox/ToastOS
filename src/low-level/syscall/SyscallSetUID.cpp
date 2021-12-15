#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallSetUID(InterruptStack *stack)
{
    uint64_t pid = stack->rsi;
    uid_t uid = (uid_t)stack->rdx;

    globalProcessManager->SetUID(pid, uid);

    return 0;
}