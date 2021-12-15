#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallGetUID(InterruptStack *stack)
{
    uint64_t pid = stack->rsi;

    return globalProcessManager->GetUID(pid);
}