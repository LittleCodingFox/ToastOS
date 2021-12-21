#include "syscall.hpp"
#include "interrupts/Interrupts.hpp"
#include "debug.hpp"
#include "Panic.hpp"

int64_t SyscallPanic(InterruptStack *stack)
{
#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: Panic", 0);
#endif

    Panic("KERNEL PANIC");

    return 0;
}