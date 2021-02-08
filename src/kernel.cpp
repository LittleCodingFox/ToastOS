#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "liballoc/liballoc.h"
#include "printf/printf.h"
#include "vtconsole/vtconsole.h"
#include "timer/Timer.hpp"
#include "KernelUtils.hpp"

vtconsole_t *console = NULL;

void PaintHandler(struct vtconsole* vtc, vtcell_t* cell, int x, int y)
{
    globalRenderer->putChar(cell->c, x * globalRenderer->fontWidth(), y * globalRenderer->fontHeight());
}

void CursorHandler (struct vtconsole* vtc, vtcursor_t* cur)
{
}

extern "C" void _start(BootInfo* bootInfo)
{
    KernelInfo kernelInfo = InitializeKernel(bootInfo);
    PageTableManager* pageTableManager = kernelInfo.pageTableManager;

    int consoleWidth = globalRenderer->width() / globalRenderer->fontWidth();
    int consoleHeight = globalRenderer->height() / globalRenderer->fontHeight();

    DEBUG_OUT("creating console: width: %d; height: %d;", consoleWidth, consoleHeight);

    console = vtconsole(consoleWidth, consoleHeight, PaintHandler, CursorHandler);

    printf("Console Test\n");

    for(;;);
}

void _putchar(char character)
{
    vtconsole_putchar(console, character);
    SerialPortOutStreamCOM1(character, NULL);
}
