#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallExitThread(InterruptStack *stack)
{
#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: thread exit", 0);
#endif

    auto process = globalProcessManager->CurrentProcess();
    
    if(process == NULL || process->isValid == false)
    {
        return 0;
    }

    globalProcessManager->ExitThread();

    return 0;
}