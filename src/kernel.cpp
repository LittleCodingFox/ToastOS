#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stivale2.h>
#include "liballoc/liballoc.h"
#include "printf/printf.h"
#include "vtconsole/vtconsole.h"
#include "timer/Timer.hpp"
#include "KernelUtils.hpp"
#include "elf/elf.hpp"
#include "filesystems/VFS.hpp"
#include "process/process.hpp"

using namespace FileSystem;

static uint8_t stack[0x100000];

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
    .tags = (uint64_t)&framebufferTag,
};

extern "C" void _start(stivale2_struct *stivale2Struct)
{
    InitializeKernel(stivale2Struct);

    FILE_HANDLE handle = vfs.OpenFile("/bin/test");

    uint64_t length = vfs.FileLength(handle);

    if(length > 0)
    {
        DEBUG_OUT("File Length: %llu", length);

        uint8_t *buffer = new uint8_t[length];

        memset(buffer, 0, length);

        if(vfs.ReadFile(handle, buffer, length) == length)
        {
            DEBUG_OUT("%s", "Executing elf");

            const char* argv[1] { NULL };

            ProcessInfo *process = globalProcessManager.LoadImage(buffer, "elf", argv);

            globalProcessManager.ExecuteProcess(process);
        }
    }

    for(;;);
}
