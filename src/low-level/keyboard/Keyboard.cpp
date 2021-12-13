#include <vtconsole/vtconsole.h>
#include "Keyboard.hpp"
#include "debug.hpp"
#include "layouts/QWERTYKeyboard.hpp"
#include "framebuffer/FramebufferRenderer.hpp"

bool isLeftShiftPressed;
bool isRightShiftPressed;

bool gotKeyboardInput = false;
char keyboardInput;

extern "C" bool GotKeyboardInput()
{
    return gotKeyboardInput;
}

extern "C" char KeyboardInput()
{
    gotKeyboardInput = false;

    return keyboardInput;
}

extern "C" void HandleKeyboardKeyPress(uint8_t scancode)
{
    switch (scancode)
    {
        case LeftShift:
            isLeftShiftPressed = true;
            return;

        case LeftShift + 0x80:
            isLeftShiftPressed = false;
            return;

        case RightShift:
            isRightShiftPressed = true;
            return;

        case RightShift + 0x80:
            isRightShiftPressed = false;
            return;

        case Enter:
            gotKeyboardInput = true;
            keyboardInput = '\n';

            return;

        case Spacebar:
            gotKeyboardInput = true;
            keyboardInput = ' ';

            return;

        case BackSpace:
            gotKeyboardInput = true;
            keyboardInput = '\b';

            return;
    }

    char ascii = QWERTYKeyboard::Translate(scancode, isLeftShiftPressed | isRightShiftPressed);

    if (ascii != 0)
    {
        gotKeyboardInput = true;
        keyboardInput = ascii;
    }
}
