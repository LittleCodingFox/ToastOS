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

    globalRenderer->Clear(0x00ff0000);

    globalRenderer->cursorPosition = {0, 0};

    globalRenderer->colour = 0;

    globalRenderer->Print("Kernel Panic");

    globalRenderer->Newline();
    globalRenderer->Newline();

    globalRenderer->Print(message);
}