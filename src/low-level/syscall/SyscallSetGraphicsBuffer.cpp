#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "framebuffer/FramebufferRenderer.hpp"
#include "user/UserAccess.hpp"

int64_t SyscallSetGraphicsBuffer(InterruptStack *stack)
{
    void *buffer = (void *)stack->rsi;

    if(!SanitizeUserPointer(buffer))
    {
        return 0;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: SetGraphicsBuffer buffer: %p", buffer);
#endif

    globalRenderer->SetGraphicsBuffer(buffer);

    return 1;
}