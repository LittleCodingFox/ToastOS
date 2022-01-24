#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "framebuffer/FramebufferRenderer.hpp"

int64_t SyscallSetGraphicsBuffer(InterruptStack *stack)
{
    void *buffer = (void *)stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: SetGraphicsBuffer buffer: %p", buffer);
#endif

    globalRenderer->SetGraphicsBuffer(buffer);

    return 1;
}