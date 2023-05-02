#include <string.h>
#include "printf.h"
#include "threading/lock.hpp"
#include "serial/Serial.hpp"
#include "vtconsole/vtconsole.h"

extern vtconsole_t *console;

AtomicLock printfLock;

extern "C"
{
    char printfBuffer[PRINTF_BUFFER_SIZE];

    char *PrintfBuffer()
    {
        return printfBuffer;
    }

    void LockPrintf()
    {
        printfLock.Lock();
    }

    void UnlockPrintf()
    {
        printfLock.Unlock();
    }

    void PrintLine(const char *buffer)
    {
        if(console != NULL)
        {
            vtconsole_write(console, buffer, strlen(buffer));
        }

        SerialCOM1.PrintLine(buffer);
    }

    void Print(const char *buffer)
    {
        if(console != NULL)
        {
            vtconsole_write(console, buffer, strlen(buffer));
        }

        SerialCOM1.Print(buffer);
    }
}