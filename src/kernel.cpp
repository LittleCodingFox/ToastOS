#include <stdint.h>
#include <stddef.h>
#include "liballoc/liballoc.h"
#include "printf/printf.h"
#include "vtconsole/vtconsole.h"
#include "KernelUtils.hpp"

vtconsole_t *console = NULL;

void PaintHandler(struct vtconsole* vtc, vtcell_t* cell, int x, int y)
{
    GlobalRenderer->PutChar(cell->c, x * GlobalRenderer->FontWidth(), y * GlobalRenderer->FontHeight());
}

void CursorHandler (struct vtconsole* vtc, vtcursor_t* cur)
{
}

extern "C" void _start(BootInfo* bootInfo)
{
    serialPortInit(SERIAL_PORT_COM1, SERIAL_PORT_SPEED_115200);

    KernelInfo kernelInfo = InitializeKernel(bootInfo);
    PageTableManager* pageTableManager = kernelInfo.pageTableManager;

    int consoleWidth = GlobalRenderer->Width() / GlobalRenderer->FontWidth();
    int consoleHeight = GlobalRenderer->Height() / GlobalRenderer->FontHeight();

    DEBUG_OUT("creating console: width: %d; height: %d;", consoleWidth, consoleHeight);

    console = vtconsole(consoleWidth, consoleHeight, PaintHandler, CursorHandler);

    DEBUG_OUT("Console buffer: %p", console->buffer);

    printf("test\n");

    for(;;);
}

void _putchar(char character)
{
    vtconsole_putchar(console, character);
    serialPortOutStream(character, NULL);
}
