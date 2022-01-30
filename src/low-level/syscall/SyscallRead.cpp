#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "filesystems/VFS.hpp"
#include "input/InputSystem.hpp"
#include "debug.hpp"
#include "errno.h"
#include "ctype.h"

using namespace FileSystem;

extern bool InputEnabled;

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
        uint8_t *ptr = (uint8_t *)buffer;

        #if USE_INPUT_SYSTEM
        InputEvent event;

        while(!globalInputSystem->Poll(&event) || event.type != TOAST_INPUT_EVENT_KEYDOWN || event.keyEvent.character == '\0')
        {
            ProcessYield();
        }

        char keyboardInput = (char)event.keyEvent.character;
        #else
        while(!GotKeyboardInput())
        {
            ProcessYield();
        }

        char keyboardInput = KeyboardInput();
        #endif

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