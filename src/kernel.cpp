#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "liballoc/liballoc.h"
#include "printf/printf.h"
#include "vtconsole/vtconsole.h"
#include "KernelUtils.hpp"

vtconsole_t *console = NULL;

void PaintHandler(struct vtconsole* vtc, vtcell_t* cell, int x, int y)
{
    globalRenderer->PutChar(cell->c, x * globalRenderer->FontWidth(), y * globalRenderer->FontHeight());
}

void CursorHandler (struct vtconsole* vtc, vtcursor_t* cur)
{
}

extern "C" void _start(BootInfo* bootInfo)
{
    KernelInfo kernelInfo = InitializeKernel(bootInfo);
    PageTableManager* pageTableManager = kernelInfo.pageTableManager;

    int consoleWidth = globalRenderer->Width() / globalRenderer->FontWidth();
    int consoleHeight = globalRenderer->Height() / globalRenderer->FontHeight();

    //DEBUG_OUT("creating console: width: %d; height: %d;", consoleWidth, consoleHeight);

    //console = vtconsole(consoleWidth, consoleHeight, PaintHandler, CursorHandler);

    //DEBUG_OUT("Console buffer: %p", console->buffer);

    printf("test\n");

    for(;;);
}

void _putchar(char character)
{
    globalRenderer->PutChar(character);
    SerialPortOutStreamCOM1(character, NULL);
}
