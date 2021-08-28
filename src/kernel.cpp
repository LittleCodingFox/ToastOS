#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stivale2.h>
#include "liballoc/liballoc.h"
#include "printf/printf.h"
#include "vtconsole/vtconsole.h"
#include "timer/Timer.hpp"
#include "KernelUtils.hpp"

static uint8_t stack[4096];

static stivale2_header_tag_framebuffer framebufferTag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        .next = 0,
    },
    .framebuffer_width = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp = 0,
};

__attribute__((section(".stivale2hdr"), used))
static struct stivale2_header stivaleHeader = {
    .entry_point = 0,
    .stack = (uintptr_t)stack + sizeof(stack),
    .flags = (1 << 1) | (1 << 2),
    .tags = (uintptr_t)&framebufferTag,
};

extern "C" void _start(stivale2_struct *stivale2Struct)
{
    DEBUG_OUT("%s", "START!");

    KernelInfo kernelInfo = InitializeKernel(stivale2Struct);
    PageTableManager* pageTableManager = kernelInfo.pageTableManager;

    for(;;);
}
