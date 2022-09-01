#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallSetUID(InterruptStack *stack)
{
    uid_t uid = stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: SetUID pid: %i uid: %i", processManager->CurrentProcess()->info->ID, uid);
#endif

    processManager->SetUID(uid);

    return 0;
}