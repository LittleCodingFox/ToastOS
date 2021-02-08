#include "Keyboard.hpp"
#include "layouts/QWERTYKeyboard.hpp"
#include "framebuffer/FramebufferRenderer.hpp"
#include <vtconsole/vtconsole.h>

bool isLeftShiftPressed;
bool isRightShiftPressed;

extern vtconsole_t *console;

void HandleKeyboardKeyPress(uint8_t scancode)
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
            vtconsole_putchar(console, '\n');

            return;

        case Spacebar:
            vtconsole_putchar(console, ' ');

            return;

        case BackSpace:
            vtconsole_putchar(console, '\b');

            return;
    }

    char ascii = QWERTYKeyboard::Translate(scancode, isLeftShiftPressed | isRightShiftPressed);

    if (ascii != 0)
    {
        vtconsole_putchar(console, ascii);
    }
}
