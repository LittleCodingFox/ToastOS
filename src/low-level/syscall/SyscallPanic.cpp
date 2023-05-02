#include "syscall.hpp"
#include "interrupts/Interrupts.hpp"
#include "debug.hpp"
#include "Panic.hpp"

int64_t SyscallPanic(InterruptStack *stack)
{
#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("%s", "Syscall: Panic");
#endif

    Panic("KERNEL PANIC");

    return 0;
}