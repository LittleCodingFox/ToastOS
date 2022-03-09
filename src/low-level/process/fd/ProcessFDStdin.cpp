#include "../Process.hpp"
#include "fcntl.h"
#include "filesystems/VFS.hpp"
#include "errno.h"
#include "input/InputSystem.hpp"
#include "keyboard/Keyboard.hpp"

extern "C" void ProcessYield();

void ProcessFDStdin::Close() {}

uint64_t ProcessFDStdin::Read(void *buffer, uint64_t length, int *error)
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

uint64_t ProcessFDStdin::Write(const void *buffer, uint64_t length, int *error)
{
    return 0;
}

int64_t ProcessFDStdin::Seek(uint64_t offset, int whence, int *error)
{
    *error = ESPIPE;

    return 0;
}

dirent *ProcessFDStdin::ReadEntries()
{
    return NULL;
}

struct stat ProcessFDStdin::Stat(int *error)
{
    return {};
}