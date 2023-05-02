#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallExitThread(InterruptStack *stack)
{
#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("%s", "Syscall: thread exit");
#endif

    auto process = processManager->CurrentProcess();
    
    if(process == NULL || process->isValid == false)
    {
        return 0;
    }

    processManager->ExitThread();

    return 0;
}