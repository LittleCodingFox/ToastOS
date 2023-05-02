#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallGetUID(InterruptStack *stack)
{
#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("%s", "Syscall: GetUID");
#endif

    auto process = processManager->CurrentProcess();

    if(process == NULL || process->isValid == false)
    {
        return 0;
    }

    return process->info->uid;
}