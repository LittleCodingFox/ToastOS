#include "keyboard.hpp"
#include "layouts/QWERTYKeyboard.hpp"
#include "framebuffer/FramebufferRenderer.hpp"

bool isLeftShiftPressed;
bool isRightShiftPressed;

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
            GlobalRenderer->Newline();
            return;

        case Spacebar:
            GlobalRenderer->PutChar(' ');
            return;

        case BackSpace:
            GlobalRenderer->ClearChar();
            return;
    }

    char ascii = QWERTYKeyboard::Translate(scancode, isLeftShiftPressed | isRightShiftPressed);

    if (ascii != 0)
    {
        GlobalRenderer->PutChar(ascii);
    }
}
