#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "interrupts/Interrupts.hpp"
#include "debug.hpp"

int64_t SyscallSetKBLayout(InterruptStack *stack)
{
    const char *name = (const char *)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: SetKBLayout name: %p", name);

    if(name != NULL)
    {
        DEBUG_OUT("Syscall: SetKBLayout name: %s", name);
    }
#endif

    if(name == NULL)
    {
        return 0;
    }

    int error = 0;

    bool result = SetKeyboardLayout(name, &error);

    if(error != 0)
    {
        return error;
    }

    return result;
}
