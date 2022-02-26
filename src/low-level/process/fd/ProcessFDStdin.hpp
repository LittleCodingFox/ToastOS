#include "../Process.hpp"
#include "filesystems/VFS.hpp"
#include "input/InputSystem.hpp"
#include "keyboard/Keyboard.hpp"
#include "fcntl.h"
#include "errno.h"

extern "C" void ProcessYield();

class ProcessFDStdin : public IProcessFD
{
public:
    virtual void Close() override {}

    virtual uint64_t Read(void *buffer, uint64_t length, int *error) override
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

    virtual uint64_t Write(const void *buffer, uint64_t length, int *error) override
    {
        return 0;
    }

    virtual int64_t Seek(uint64_t offset, int whence, int *error) override
    {
        *error = ESPIPE;

        return 0;
    }

    virtual dirent *ReadEntries() override
    {
        return NULL;
    }

    virtual struct stat Stat(int *error) override
    {
        return {};
    }
};