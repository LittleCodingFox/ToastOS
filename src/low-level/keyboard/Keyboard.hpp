#pragma once
#include <stdint.h>

#ifndef __cplusplus
#   include <stdbool.h>
#endif

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

void InitializeKeyboard();
void HandleKeyboardKeyPress(uint8_t scancode);
void SetKeyboardLayout(const char *name);
const char *GetKeyboardLayoutName();

#if !USE_INPUT_SYSTEM
bool GotKeyboardInput();
char KeyboardInput();
#endif

#ifdef __cplusplus
}
#endif
