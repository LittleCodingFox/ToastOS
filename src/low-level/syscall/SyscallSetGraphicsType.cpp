#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "framebuffer/FramebufferRenderer.hpp"

int64_t SyscallSetGraphicsType(InterruptStack *stack)
{
    int type = stack->rsi;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: SetGraphicsType type: %i", type);
#endif

    globalRenderer->SetGraphicsType(type);

    return 1;
}