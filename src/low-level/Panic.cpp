#include "Panic.hpp"
#include "debug.hpp"
#include "framebuffer/FramebufferRenderer.hpp"
#include "stacktrace/stacktrace.hpp"

void Panic(const char* panicMessage)
{
    DEBUG_OUT("Panic: %s", panicMessage);

    kernel_dump_stacktrace();

    GlobalRenderer->Clear(0x00ff0000);

    GlobalRenderer->CursorPosition = {0, 0};

    GlobalRenderer->Colour = 0;

    GlobalRenderer->Print("Kernel Panic");

    GlobalRenderer->Newline();
    GlobalRenderer->Newline();

    GlobalRenderer->Print(panicMessage);
}