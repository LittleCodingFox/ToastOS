#include "../stdio.h"

#if IS_LIBC
#   include "../sys/syscall.h"
#   define SYSCALL_READ 2
#else
#   include "printf/printf.h"
#   include "low-level/keyboard/Keyboard.hpp"
#endif

size_t read(int fd, const void *buffer, size_t count)
{
#if IS_LIBC
    return syscall(SYSCALL_READ, fd, buffer, count);
#else
    if(fd == 0)
    {
        while(!GotKeyboardInput());
        char keyboardInput = (char)KeyboardInput();

        uint8_t *ptr = (uint8_t *)buffer;

        *ptr = keyboardInput;

        return 1;
    }
    else if(fd > 2)
    {
        //TODO
    }

    return 0;
#endif
}