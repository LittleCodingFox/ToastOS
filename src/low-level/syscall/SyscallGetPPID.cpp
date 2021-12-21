#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallGetPPID(InterruptStack *stack)
{
    ProcessInfo *process = globalProcessManager->CurrentProcess();

    return process->ppid;
}