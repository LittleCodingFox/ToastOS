#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stivale2.h>
#include "fcntl.h"
#include "liballoc/liballoc.h"
#include "printf/printf.h"
#include "vtconsole/vtconsole.h"
#include "timer/Timer.hpp"
#include "KernelUtils.hpp"
#include "elf/elf.hpp"
#include "filesystems/VFS.hpp"
#include "process/Process.hpp"
#include "framebuffer/FramebufferRenderer.hpp"
#include "smp/SMP.hpp"

static uint8_t stack[SMP_STACK_SIZE];

const char *startAppPath = "/bin/init";

const char *args[] = {};

const char *cwd = "/home/toast/";

static stivale2_header_tag_framebuffer framebufferTag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        .next = 0,
    },
    .framebuffer_width = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp = 0,
};

static stivale2_header_tag_smp smpTag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_SMP_ID,
        .next = (uint64_t)&framebufferTag,
    },
    .flags = 0,
};

__attribute__((section(".stivale2hdr"), used))
static struct stivale2_header stivaleHeader = {
    .entry_point = 0,
    .stack = (uintptr_t)stack + sizeof(stack),
    .flags = (1 << 1) | (1 << 2),
    .tags = (uint64_t)&smpTag,
};

void KernelTask()
{
    //Idle process
    for(;;)
    {
        asm volatile("hlt");
    }
}

extern "C" void _start(stivale2_struct *stivale2Struct)
{
    InitializeKernel(stivale2Struct);

    printf("Starting app at %s\n", startAppPath);

    int error = 0;

    FILE_HANDLE handle = vfs->OpenFile(startAppPath, O_RDONLY, NULL, &error);

    uint64_t length = vfs->FileLength(handle);

    if(length > 0)
    {
        uint8_t *buffer = new uint8_t[length];

        if(vfs->ReadFile(handle, buffer, length, &error) == length)
        {
            vfs->CloseFile(handle);

            int size = sizeof(args) / sizeof(args[0]);

            char **argv = new char*[size + 2];

            argv[0] = (char *)startAppPath;

            for(int i = 0; i < size; i++)
            {
                argv[i + 1] = (char *)args[i];
            }

            argv[size + 1] = NULL;

            const char *envp[] { "HOME=/home/toast", NULL };

            processManager->CreateFromEntryPoint((uint64_t)KernelTask, "KernelTask", "/home/toast/", PROCESS_PERMISSION_KERNEL);
            processManager->LoadImage(buffer, "elf", (const char**)argv, envp, cwd, PROCESS_PERMISSION_USER);
        }
        else
        {
            printf("Failed to open app at %s: Failed to read file\n", startAppPath);
        }
    }
    else
    {
        printf("Failed to open app at %s: File not found or empty\n", startAppPath);
    }

    for(;;);
}
