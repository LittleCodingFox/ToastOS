#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallGetPID(InterruptStack *stack)
{
    ProcessInfo *process = globalProcessManager->CurrentProcess();

    return process->ID;
}