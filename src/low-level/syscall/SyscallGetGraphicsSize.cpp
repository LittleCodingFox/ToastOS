#include "syscall.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "framebuffer/FramebufferRenderer.hpp"
#include "user/UserAccess.hpp"

int64_t SyscallGetGraphicsSize(InterruptStack *stack)
{
    int *width = (int *)stack->rsi;
    int *height = (int *)stack->rdx;
    int *bpp = (int *)stack->rcx;

    if(!SanitizeUserPointer(width) || !SanitizeUserPointer(height) || !SanitizeUserPointer(bpp))
    {
        return 0;
    }

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: GetGraphicsSize width: %p height %p bpp %p", width, height, bpp);
#endif

    if(width == NULL || height == NULL || bpp == NULL)
    {
        return 0;       
    }

    *width = globalRenderer->Width();
    *height = globalRenderer->Height();
    *bpp = globalRenderer->BitsPerPixel();

    return 1;
}