#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallTCBSet(InterruptStack *stack)
{
    ProcessInfo *process = globalProcessManager->CurrentProcess();

    uint64_t base = stack->rsi;

    process->fsBase = base;

    globalProcessManager->LoadFSBase(base);

    return 0;
}