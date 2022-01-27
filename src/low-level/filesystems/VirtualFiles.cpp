#include "VFS.hpp"
#include "keyboard/Keyboard.hpp"
#include "input/InputSystem.hpp"

namespace FileSystem
{
    uint64_t ReadDevTTY(void *buffer, uint64_t cursor, uint64_t length)
    {
        InputEvent event;

        if(!globalInputSystem->Poll(&event) || event.type != TOAST_INPUT_EVENT_KEYDOWN)
        {
            return 0;
        }

        char keyboardInput = (char)event.keyEvent.character;

        uint8_t *ptr = (uint8_t *)buffer;

        *ptr = keyboardInput;

        printf("%c", keyboardInput);

        return 1;
    }

    uint64_t WriteDevTTY(const void *buffer, uint64_t cursor, uint64_t length)
    {
        printf("%.*s", length, buffer);

        return length;
    }

    void InitializeVirtualFiles()
    {
        VirtualFile devtty;
        devtty.path = "/dev/tty";
        devtty.type = FILE_HANDLE_CHARDEVICE;
        devtty.read = ReadDevTTY;
        devtty.write = WriteDevTTY;

        vfs->AddVirtualFile(devtty);
    }
}