#include "Serial.hpp"
#include <ports/Ports.hpp>
#include "stacktrace/stacktrace.hpp"

Serial SerialCOM1;

#define SERIAL_PORT_DATA_PORT(base)             (base)
#define SERIAL_PORT_FIFO_COMMAND_PORT(base)     (base + 2)
#define SERIAL_PORT_LINE_COMMAND_PORT(base)     (base + 3)
#define SERIAL_PORT_MODEM_COMMAND_PORT(base)    (base + 4)
#define SERIAL_PORT_LINE_STATUS_PORT(base)      (base + 5)

#define SERIAL_PORT_LINE_ENABLE_DLAB            0x80

extern "C" void DumpSerialString(const char *msg)
{
    SerialCOM1.PrintLine(msg);
}

void InitializeSerial()
{
    SerialCOM1.port = SerialPorts::COM1;
    SerialCOM1.speed = SerialSpeeds::Speed115200;
    SerialCOM1.initialized = false;

    SerialCOM1.Initialize();
}

bool SerialPortIsTransmitFIFOEmpty(uint16_t port)
{
    return (inport8(SERIAL_PORT_LINE_STATUS_PORT(port)) & 0x20);
}

void Serial::Initialize()
{
    if(initialized)
    {
        return;
    }

    initialized = true;

    outport8(SERIAL_PORT_LINE_COMMAND_PORT(port), SERIAL_PORT_LINE_ENABLE_DLAB);
    outport8(SERIAL_PORT_DATA_PORT(port), (speed >> 8) & 0x00FF);
    outport8(SERIAL_PORT_DATA_PORT(port), speed & 0x00FF);

    outport8(SERIAL_PORT_LINE_COMMAND_PORT(port), 0x03);
    outport8(SERIAL_PORT_FIFO_COMMAND_PORT(port), 0xC7);
    outport8(SERIAL_PORT_MODEM_COMMAND_PORT(port), 0x08);
}

void Serial::Write(char c)
{
    ScopedLock lock(this->lock);

    Initialize();

    while(SerialPortIsTransmitFIFOEmpty(port) == false);

    outport8(port, c);
}

void Serial::WriteNoLock(char c)
{
    Initialize();

    while(SerialPortIsTransmitFIFOEmpty(port) == false);

    outport8(port, c);
}

void Serial::Print(const char *string)
{
    while(*string)
    {
        Write(*string);

        string++;
    }
}

void Serial::PrintNoLock(const char *string)
{
    while(*string)
    {
        WriteNoLock(*string);

        string++;
    }
}

void Serial::PrintLine(const char *string)
{
    Print(string);
    Print("\n");
}

void Serial::PrintLineNoLock(const char *string)
{
    PrintNoLock(string);
    PrintNoLock("\n");
}

extern "C" void SerialPortOutStreamCOM1(char character, void *arg)
{
    SerialCOM1.Write(character);
}

extern "C" void SerialPortOutStreamCOM1NoLock(char character, void *arg)
{
    SerialCOM1.WriteNoLock(character);
}
