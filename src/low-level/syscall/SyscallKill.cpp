#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallKill(InterruptStack *stack)
{
    uint64_t pid = stack->rsi;
    int signal = (int)stack->rdx;

    globalProcessManager->Kill(pid, signal);

    return 0;
}