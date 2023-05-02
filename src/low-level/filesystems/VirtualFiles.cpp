#include "VFS.hpp"
#include "keyboard/Keyboard.hpp"
#include "input/InputSystem.hpp"

uint64_t ReadDevTTY(void *buffer, uint64_t cursor, uint64_t length, int *error, void *userdata)
{
    InputEvent event;

    if(!globalInputSystem->Poll(&event) || event.type != TOAST_INPUT_EVENT_KEYDOWN || event.keyEvent.character == '\0')
    {
        return 0;
    }

    char keyboardInput = (char)event.keyEvent.character;

    uint8_t *ptr = (uint8_t *)buffer;

    *ptr = keyboardInput;

    printf("%c", keyboardInput);

    return 1;
}

uint64_t WriteDevTTY(const void *buffer, uint64_t cursor, uint64_t length, int *error, void *userdata)
{
    printf("%.*s", (int)length, (char *)buffer);

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
