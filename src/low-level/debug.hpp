#pragma once
#include "support/printf.h"
#if __cplusplus
#include "serial/Serial.hpp"
#endif

#define KERNEL_DEBUG_SYSCALLS   0
#define DEBUG_PROCESSES         0
#define DEBUG_PROCESSES_EXTRA   0

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
#define DEBUG_OUT(msg, ...)    { char DEBUG__BUFFER[10240]; snprintf(DEBUG__BUFFER, sizeof(DEBUG__BUFFER), msg "\n", __VA_ARGS__); SerialCOM1.Print(DEBUG__BUFFER); }
#define DEBUG_OUT_NOLOCK(msg, ...)  { char DEBUG__BUFFER[10240]; snprintf(DEBUG__BUFFER, sizeof(DEBUG__BUFFER), msg "\n", __VA_ARGS__); SerialCOM1.PrintNoLock(DEBUG__BUFFER); }
#else
#define DEBUG_OUT(msg, ...)
#define DEBUG_OUT_NOLOCK(msg, ...)
#endif
