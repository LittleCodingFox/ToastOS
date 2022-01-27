#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "input/InputSystem.hpp"
#include "debug.hpp"
#include "errno.h"
#include "ctype.h"

using namespace FileSystem;

int64_t SyscallRead(InterruptStack *stack)
{
    int fd = stack->rsi;
    void *buffer = (void *)stack->rdx;
    size_t count = (size_t)stack->rcx;

#if KERNEL_DEBUG_SYSCALLS
    DEBUG_OUT("Syscall: Read fd: %i buffer: %p count: %llu", fd, buffer, count);
#endif

    if(fd == 0) //stdin
    {
        InputEvent event;

        while(!globalInputSystem->Poll(&event) || event.type != TOAST_INPUT_EVENT_KEYDOWN)
        {
            ProcessYield();
        }

        char keyboardInput = (char)event.keyEvent.character;

        uint8_t *ptr = (uint8_t *)buffer;

        *ptr = keyboardInput;

        printf("%c", keyboardInput);

        return 1;
    }
    else if(fd >= 3) //actual files
    {
        FILE_HANDLE handle = fd - 3;

        if(vfs->FileType(handle) == FILE_HANDLE_UNKNOWN)
        {
            return -ENOENT;
        }

        return vfs->ReadFile(handle, buffer, count);
    }

    return 0;
}