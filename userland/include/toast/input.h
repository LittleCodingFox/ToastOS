#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#if __cplusplus
extern "C"
{
#endif

enum InputKey
{
    TOAST_KEY_A,
    TOAST_KEY_B,
    TOAST_KEY_C,
    TOAST_KEY_D,
    TOAST_KEY_E,
    TOAST_KEY_F,
    TOAST_KEY_G,
    TOAST_KEY_H,
    TOAST_KEY_I,
    TOAST_KEY_J,
    TOAST_KEY_K,
    TOAST_KEY_L,
    TOAST_KEY_M,
    TOAST_KEY_N,
    TOAST_KEY_O,
    TOAST_KEY_P,
    TOAST_KEY_Q,
    TOAST_KEY_R,
    TOAST_KEY_S,
    TOAST_KEY_T,
    TOAST_KEY_U,
    TOAST_KEY_V,
    TOAST_KEY_Y,
    TOAST_KEY_Z,
    TOAST_KEY_1,
    TOAST_KEY_2,
    TOAST_KEY_3,
    TOAST_KEY_4,
    TOAST_KEY_5,
    TOAST_KEY_6,
    TOAST_KEY_7,
    TOAST_KEY_8,
    TOAST_KEY_9,
    TOAST_KEY_0,
    TOAST_KEY_LEFT_SHIFT,
    TOAST_KEY_RIGHT_SHIFT,
    TOAST_KEY_LEFT_CONTROL,
    TOAST_KEY_RIGHT_CONTROL,
    TOAST_KEY_LEFT_ARROW,
    TOAST_KEY_RIGHT_ARROW,
    TOAST_KEY_UP_ARROW,
    TOAST_KEY_DOWN_ARROW,
    TOAST_KEY_SPACE,
    TOAST_KEY_BACKSPACE,
    TOAST_KEY_RETURN,
    TOAST_KEY_ESCAPE,
    TOAST_KEY_F1,
    TOAST_KEY_F2,
    TOAST_KEY_F3,
    TOAST_KEY_F4,
    TOAST_KEY_F5,
    TOAST_KEY_F6,
    TOAST_KEY_F7,
    TOAST_KEY_F8,
    TOAST_KEY_F9,
    TOAST_KEY_F10,
    TOAST_KEY_F11,
    TOAST_KEY_F12,
    TOAST_KEY_TAB,
    TOAST_KEY_CAPS_LOCK
};

enum InputEventType
{
    TOAST_INPUT_EVENT_NONE,
    TOAST_INPUT_EVENT_KEYDOWN,
    TOAST_INPUT_EVENT_KEYUP
};

enum InputModifierType
{
    TOAST_INPUT_MODIFIER_LSHIFT = (1 << 1),
    TOAST_INPUT_MODIFIER_RSHIFT = (1 << 2),
    TOAST_INPUT_MODIFIER_LCONTROL = (1 << 3),
    TOAST_INPUT_MODIFIER_RCONTROL = (1 << 4)
};

struct InputKeyEvent
{
    uint8_t key;
    uint16_t character;
    uint8_t modifiers;
};

struct InputEvent
{
    uint32_t type;
    struct InputKeyEvent keyEvent;
};

bool ToastInputPollEvent(struct InputEvent *event);

#if __cplusplus
}
#endif