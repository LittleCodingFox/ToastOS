#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallGetGID(InterruptStack *stack)
{
    uint64_t pid = stack->rsi;

    return globalProcessManager->GetGID(pid);
}