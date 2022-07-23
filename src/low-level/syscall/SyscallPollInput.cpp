#include "syscall.hpp"
#include "input/InputSystem.hpp"
#include "interrupts/Interrupts.hpp"
#include "debug.hpp"
#include "user/UserAccess.hpp"

int64_t SyscallPollInput(InterruptStack *stack)
{
    InputEvent *event = (InputEvent *)stack->rsi;

    if(!SanitizeUserPointer(event))
    {
        return 0;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: poll input: %p", event);
#endif

    return globalInputSystem->Poll(event);
} 