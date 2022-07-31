#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limine.h>
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

//static uint8_t stack[0x100000];

const char *startAppPath = "/usr/bin/bash";

const char *args[] =
{
    "-i", "-l"
};

const char *cwd = "/home/toast/";

static volatile limine_framebuffer_request framebuffer = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
};

static volatile limine_memmap_request memmap = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
};

static volatile limine_module_request modules = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0,
};

void KernelTask()
{
    //Idle process
    for(;;)
    {
        asm volatile("hlt");
    }
}

extern "C" void _start()
{
    InitializeKernel(&framebuffer, &memmap, &modules);

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
