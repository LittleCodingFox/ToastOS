#include "Mouse.hpp"
#include "input/InputSystem.hpp"
#include "ports/Ports.hpp"
#include "framebuffer/FramebufferRenderer.hpp"

#define PS2LeftButton   0b00000001
#define PS2MiddleButton 0b00000100
#define PS2RightButton  0b00000010
#define PS2XSign        0b00010000
#define PS2YSign        0b00100000
#define PS2XOverflow    0b01000000
#define PS2YOverflow    0b10000000

int mouseX = 0, mouseY = 0;
int previousX = 0, previousY = 0;
int mouseButtons = 0;
uint8_t mouseCycle = 0;
uint8_t mousePacket[4];
bool mousePacketReady = false;

void MouseWait()
{
    uint64_t timeout = 100000;

    while (timeout--)
    {
        if ((inport8(0x64) & 0b10) == 0)
        {
            return;
        }
    }
}

void MouseWaitInput()
{
    uint64_t timeout = 100000;

    while (timeout--)
    {
        if (inport8(0x64) & 0b1)
        {
            return;
        }
    }
}

void MouseWrite(uint8_t value)
{
    MouseWait();

    outport8(0x64, 0xD4);

    MouseWait();

    outport8(0x60, value);
}

uint8_t MouseRead()
{
    MouseWaitInput();

    return inport8(0x60);
}

void HandleMouse(uint8_t data)
{
    switch(mouseCycle)
    {
        case 0:

            if(mousePacketReady)
            {
                break;
            }

            if((data & 0b00001000) == 0)
            {
                break;
            }

            mousePacket[0] = data;
            mouseCycle++;

            break;

        case 1:

            if(mousePacketReady)
            {
                break;
            }

            mousePacket[1] = data;

            mouseCycle++;

            break;

        case 2:

            if(mousePacketReady)
            {
                break;
            }

            mousePacket[2] = data;
            mousePacketReady = true;
            mouseCycle = 0;

            ProcessMousePacket();

            break;
    }
}

void AddMouseMoveEvent()
{
    InputEvent event = {
        .type = TOAST_INPUT_EVENT_MOUSEMOVE,
        .mouseEvent = {
            .x = mouseX,
            .y = mouseY,
            .buttons = mouseButtons,
        }
    };

    globalInputSystem->AddEvent(event);
}

void AddMouseButtonPressEvent(int button)
{
    InputEvent event = {
        .type = TOAST_INPUT_EVENT_MOUSEDOWN,
        .mouseEvent = {
            .x = mouseX,
            .y = mouseY,
            .buttons = button,
        }
    };

    globalInputSystem->AddEvent(event);
}

void AddMouseButtonReleaseEvent(int button)
{
    InputEvent event = {
        .type = TOAST_INPUT_EVENT_MOUSEUP,
        .mouseEvent = {
            .x = mouseX,
            .y = mouseY,
            .buttons = button,
        }
    };

    globalInputSystem->AddEvent(event);
}

void ProcessMousePacket()
{
    if(mousePacketReady == false)
    {
        return;
    }

    auto previousX = mouseX;
    auto previousY = mouseY;

    bool xNegative = false, yNegative = false, xOverflow = false, yOverflow = false;

    if(mousePacket[0] & PS2XSign)
    {
        xNegative = true;
    }

    if(mousePacket[0] & PS2YSign)
    {
        yNegative = true;
    }

    if(mousePacket[0] & PS2XOverflow)
    {
        xOverflow = true;
    }

    if(mousePacket[0] & PS2YOverflow)
    {
        yOverflow = true;
    }

    if(xNegative == false)
    {
        mouseX += mousePacket[1];

        if(xOverflow)
        {
            mouseX += 255;
        }
    }
    else
    {
        mousePacket[1] = 256 - mousePacket[1];

        mouseX -= mousePacket[1];

        if(xOverflow)
        {
            mouseX -= 255;
        }
    }

    if(yNegative == false)
    {
        mouseY -= mousePacket[2];

        if(yOverflow)
        {
            mouseY -= 255;
        }
    }
    else
    {
        mousePacket[2] = 256 - mousePacket[2];

        mouseY += mousePacket[2];

        if(yOverflow)
        {
            mouseY += 255;
        }
    }

    if(mouseX < 0)
    {
        mouseX = 0;
    }

    if(mouseY < 0)
    {
        mouseY = 0;
    }

    if(mouseX >= globalRenderer->Width())
    {
        mouseX = globalRenderer->Width() - 1;
    }
    
    if(mouseY >= globalRenderer->Height())
    {
        mouseY = globalRenderer->Height() - 1;
    }

    if(mousePacket[0] & PS2LeftButton)
    {
        if((mouseButtons & TOAST_INPUT_MOUSE_BUTTON_LEFT) == 0)
        {
            AddMouseButtonPressEvent(TOAST_INPUT_MOUSE_BUTTON_LEFT);
        }

        mouseButtons |= TOAST_INPUT_MOUSE_BUTTON_LEFT;
    }
    else
    {
        if((mouseButtons & TOAST_INPUT_MOUSE_BUTTON_LEFT) != 0)
        {
            AddMouseButtonReleaseEvent(TOAST_INPUT_MOUSE_BUTTON_LEFT);
        }

        mouseButtons &= ~TOAST_INPUT_MOUSE_BUTTON_LEFT;
    }

    if(mousePacket[0] & PS2MiddleButton)
    {
        if((mouseButtons & TOAST_INPUT_MOUSE_BUTTON_MIDDLE) == 0)
        {
            AddMouseButtonPressEvent(TOAST_INPUT_MOUSE_BUTTON_MIDDLE);
        }

        mouseButtons |= TOAST_INPUT_MOUSE_BUTTON_MIDDLE;
    }
    else
    {
        if((mouseButtons & TOAST_INPUT_MOUSE_BUTTON_MIDDLE) != 0)
        {
            AddMouseButtonReleaseEvent(TOAST_INPUT_MOUSE_BUTTON_MIDDLE);
        }

        mouseButtons &= ~TOAST_INPUT_MOUSE_BUTTON_MIDDLE;
    }

    if(mousePacket[0] & PS2RightButton)
    {
        if((mouseButtons & TOAST_INPUT_MOUSE_BUTTON_RIGHT) == 0)
        {
            AddMouseButtonPressEvent(TOAST_INPUT_MOUSE_BUTTON_RIGHT);
        }

        mouseButtons |= TOAST_INPUT_MOUSE_BUTTON_RIGHT;
    }
    else
    {
        if((mouseButtons & TOAST_INPUT_MOUSE_BUTTON_RIGHT) != 0)
        {
            AddMouseButtonReleaseEvent(TOAST_INPUT_MOUSE_BUTTON_RIGHT);
        }

        mouseButtons &= ~TOAST_INPUT_MOUSE_BUTTON_RIGHT;
    }

    if(previousX != mouseX || previousY != mouseY)
    {
        AddMouseMoveEvent();
    }

    mousePacketReady = false;
}

extern "C" void InitializeMouse()
{
    outport8(0x64, 0xA8);

    MouseWait();

    outport8(0x64, 0x20);

    MouseWaitInput();

    uint8_t status = inport8(0x60);

    status |= 0b10;

    MouseWait();

    outport8(0x64, 0x60);

    MouseWait();

    outport8(0x60, status);

    MouseWrite(0xF6);
    MouseRead();

    MouseWrite(0xF4);

    MouseRead();
}

extern "C" int GetMouseX()
{
    return mouseX;
}

extern "C" int GetMouseY()
{
    return mouseY;
}

extern "C" int GetMouseButtons()
{
    return mouseButtons;
}
