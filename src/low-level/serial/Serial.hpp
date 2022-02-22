#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include "lock.hpp"

enum SerialPorts
{
    COM1 = 0x3F8,
};

enum SerialSpeeds
{
    Speed115200 = 1,
    Speed57600  = 2,
    Speed38400  = 3,
};

class Serial
{
private:
    AtomicLock lock;

public:
    uint16_t port;
    uint16_t speed;
    bool initialized = false;

    void Initialize();

    void Print(const char *string);
    void PrintLine(const char *string);
    void Write(char c);
    void WriteNoLock(char c);
    void PrintNoLock(const char *string);
    void PrintLineNoLock(const char *string);
};

extern Serial SerialCOM1;

extern "C" void SerialPortOutStreamCOM1(char character, void *arg);

void InitializeSerial();

#endif
