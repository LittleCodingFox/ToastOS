#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallSigaction(InterruptStack *stack)
{
    int signum = (int)stack->rsi;
    struct sigaction *act = (struct sigaction *)stack->rdx;
    struct sigaction *oldact = (struct sigaction *)stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: sigaction signum %i act %p oldact %p", signum, act, oldact);
#endif

    processManager->Sigaction(signum, act, oldact);

    return 0;
}