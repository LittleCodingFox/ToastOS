#pragma once
#include "printf/printf.h"

#if __cplusplus
extern "C" 
#endif
void SerialPortOutStreamCOM1(char character, void *arg);

#ifdef KERNEL_DEBUG
#define DEBUG_OUT(msg, ...)  fctprintf(&SerialPortOutStreamCOM1, NULL, msg "\n", __VA_ARGS__);
#else
#define DEBUG_OUT(msg, ...)
#endif
