#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "liballoc/liballoc.h"
#include "printf/printf.h"
#include "vtconsole/vtconsole.h"
#include "timer/Timer.hpp"
#include "KernelUtils.hpp"

extern "C" void _start(BootInfo* bootInfo)
{
    KernelInfo kernelInfo = InitializeKernel(bootInfo);
    PageTableManager* pageTableManager = kernelInfo.pageTableManager;

    for(;;);
}
