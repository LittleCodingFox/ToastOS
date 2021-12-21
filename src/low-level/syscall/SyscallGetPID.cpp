#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallGetPID(InterruptStack *stack)
{
#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: GetPID", 0);
#endif

    ProcessInfo *process = globalProcessManager->CurrentProcess();

    return process->ID;
}