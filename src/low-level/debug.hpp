#pragma once
#include "printf/printf.h"
#include <serial/Serial.hpp>

#ifdef KERNEL_DEBUG
#define DEBUG_OUT(msg, ...)  fctprintf(&SerialPortOutStreamCOM1, NULL, msg "\n", __VA_ARGS__);
#else
#define DEBUG_OUT(msg, ...)
#endif
