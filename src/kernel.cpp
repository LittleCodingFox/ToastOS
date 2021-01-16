#include "KernelUtils.hpp"

extern "C" void _start(BootInfo* bootInfo)
{
    serialPortInit(SERIAL_PORT_COM1, SERIAL_PORT_SPEED_115200);

    KernelInfo kernelInfo = InitializeKernel(bootInfo);
    PageTableManager* pageTableManager = kernelInfo.pageTableManager;

    DEBUG_OUT("Global Renderer ptr: %p", GlobalRenderer);

    GlobalRenderer->Print("Kernel Initialized Successfully");

    while(true);
}

void _putchar(char character)
{
    serialPortOutStream(character, NULL);
}
