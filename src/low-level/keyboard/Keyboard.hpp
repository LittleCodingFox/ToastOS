#pragma once
#include <stdint.h>

#ifndef __cplusplus
#   include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void HandleKeyboardKeyPress(uint8_t scancode);
bool GotKeyboardInput();
char KeyboardInput();

#ifdef __cplusplus
}
#endif
