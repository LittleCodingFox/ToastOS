#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

int64_t SyscallSigaction(InterruptStack *stack)
{
    int signum = (int)stack->rsi;
    sigaction *act = (sigaction *)stack->rdx;
    sigaction *oldact = (sigaction *)stack->rcx;

    globalProcessManager->Sigaction(signum, act, oldact);

    return 0;
}