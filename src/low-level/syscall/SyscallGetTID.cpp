#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallGetTID(InterruptStack *stack)
{
#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("%s", "Syscall: GetTID");
#endif

    auto thread = processManager->CurrentThread();

    if(thread == NULL)
    {
        return 0;
    }

    return thread->tid;
}