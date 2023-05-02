#include "Panic.hpp"
#include "debug.hpp"
#include "framebuffer/FramebufferRenderer.hpp"
#include "stacktrace/stacktrace.hpp"
#include "support/printf.h"

extern "C" void Panic(const char* format, ...)
{
    char message[10240];

    va_list arg;
    va_start(arg, format);
    vsnprintf(message, sizeof(message), format, arg);
    va_end(arg);

    DEBUG_OUT("Panic: %s", message);

    KernelDumpStacktrace();

    globalRenderer->Clear(0x0000AAAA);

    char buffer[10240];

    snprintf(buffer, sizeof(buffer), "Kernel Panic\n\n%s", message);

    psf2RenderText(0, 0, buffer, globalRenderer->GetFont(), 0x00005555, globalRenderer);

    globalRenderer->SwapBuffers();

    for(;;);
}
