#include "Panic.hpp"
#include "debug.hpp"
#include "framebuffer/FramebufferRenderer.hpp"
#include "stacktrace/stacktrace.hpp"

void Panic(const char* panicMessage)
{
    DEBUG_OUT("Panic: %s", panicMessage);

    kernel_dump_stacktrace();

    globalRenderer->Clear(0x00ff0000);

    globalRenderer->cursorPosition = {0, 0};

    globalRenderer->colour = 0;

    globalRenderer->Print("Kernel Panic");

    globalRenderer->Newline();
    globalRenderer->Newline();

    globalRenderer->Print(panicMessage);
}