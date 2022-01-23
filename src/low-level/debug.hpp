#pragma once
#include "printf/printf.h"
#if __cplusplus
#include "serial/Serial.hpp"
#endif

#define KERNEL_DEBUG_SYSCALLS 0

#if __cplusplus
extern "C" 
#endif
void SerialPortOutStreamCOM1(char character, void *arg);
#if __cplusplus
extern "C" 
#endif
void SerialPortOutStreamCOM1NoLock(char character, void *arg);

#if __cplusplus
extern "C" 
#endif
void DumpSerialString(const char *msg);

#ifdef KERNEL_DEBUG
#define DEBUG_OUT(msg, ...)    { fctprintf(&SerialPortOutStreamCOM1, NULL, msg "\n", __VA_ARGS__); }
#define DEBUG_OUT_NOLOCK(msg, ...)  { fctprintf(&SerialPortOutStreamCOM1NoLock, NULL, msg "\n", __VA_ARGS__); }
#else
#define DEBUG_OUT(msg, ...)
#define DEBUG_OUT_NOLOCK(msg, ...)
#endif
