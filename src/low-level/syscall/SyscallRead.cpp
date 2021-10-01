#include "syscall.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "debug.hpp"

size_t SyscallRead(int fd, void *buffer, size_t count)
{
    if(fd == 0) //stdin
    {
        while(!GotKeyboardInput())
        {
            ProcessYield();
        }

        char keyboardInput = (char)KeyboardInput();

        uint8_t *ptr = (uint8_t *)buffer;

        *ptr = keyboardInput;

        return 1;
    }
    else if(fd > 2) //actual files
    {
        //TODO
    }

    return 0;
}