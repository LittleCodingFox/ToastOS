#include <stdio.h>
#include "Panic.hpp"
#include "debug.hpp"
#include "framebuffer/FramebufferRenderer.hpp"
#include "stacktrace/stacktrace.hpp"

void Panic(const char* format, ...)
{
    char message[10240];

    va_list arg;
    va_start(arg, format);
    vsnprintf(message, sizeof(message), format, arg);
    va_end(arg);

    DEBUG_OUT("Panic: %s", message);

    kernelDumpStacktrace();

    globalRenderer->clear(0x00ff0000);

    char buffer[10240];

    snprintf(buffer, sizeof(buffer), "Kernel Panic\n\n%s", message);

    psf2RenderText(0, 0, buffer, globalRenderer->getFont(), 0xFFFFFFFF, globalRenderer);

    globalRenderer->swapBuffers();

    for(;;);
}
