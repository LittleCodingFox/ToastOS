#pragma once
#include <stdint.h>

#ifndef __cplusplus
#   include <stdbool.h>
#endif

#include <kernel.h>

#include "../../../userland/include/toast/input.h"

#ifdef __cplusplus
extern "C" {
#endif

void InitializeMouse();
int GetMouseX();
int GetMouseY();
int GetMouseButtons();
void ProcessMousePacket();
void HandleMouse(uint8_t data);

#ifdef __cplusplus
}
#endif
