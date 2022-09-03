#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "errno.h"

int64_t SyscallIsATTY(InterruptStack *stack)
{
    int fd = (int)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: isatty fd: %i", fd);
#endif

    auto current = processManager->CurrentProcess();

    if(current == NULL || current->isValid == false)
    {
        return 0;
    }

    auto procfd = current->info->GetFD(fd);

    if(procfd == NULL || procfd->isValid == false || procfd->impl == NULL)
    {
        return 0;
    }

    return procfd->type == PROCESS_FD_STDOUT || procfd->type == PROCESS_FD_STDIN;
}