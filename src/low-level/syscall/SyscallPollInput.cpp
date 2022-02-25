#include "syscall.hpp"
#include "input/InputSystem.hpp"
#include "interrupts/Interrupts.hpp"
#include "debug.hpp"

int64_t SyscallPollInput(InterruptStack *stack)
{
    InputEvent *event = (InputEvent *)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: poll input: %p", event);
#endif

    return globalInputSystem->Poll(event);
} 