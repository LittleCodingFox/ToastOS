#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

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

    uint16_t port;
    uint16_t speed;
    bool initialized = false;

    void Initialize();

public:

    Serial(uint16_t port, uint16_t speed);

    void Print(const char *string);
    void PrintLine(const char *string);
    void Write(char c);

};

extern Serial SerialCOM1;

extern "C" void SerialPortOutStreamCOM1(char character, void *arg);

#endif
