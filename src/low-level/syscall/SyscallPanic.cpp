#include "syscall.hpp"
#include "interrupts/Interrupts.hpp"
#include "debug.hpp"
#include "Panic.hpp"

int64_t SyscallPanic(InterruptStack *stack)
{
    Panic("KERNEL PANIC");

    return 0;
}