#include "Serial.hpp"
#include <ports/Ports.hpp>

Serial SerialCOM1(SerialPorts::COM1, SerialSpeeds::Speed115200);

#define SERIAL_PORT_DATA_PORT(base)             (base)
#define SERIAL_PORT_FIFO_COMMAND_PORT(base)     (base + 2)
#define SERIAL_PORT_LINE_COMMAND_PORT(base)     (base + 3)
#define SERIAL_PORT_MODEM_COMMAND_PORT(base)    (base + 4)
#define SERIAL_PORT_LINE_STATUS_PORT(base)      (base + 5)

#define SERIAL_PORT_LINE_ENABLE_DLAB            0x80

bool serialPortIsTransmitFIFOEmpty(uint16_t port)
{
    return (inport8(SERIAL_PORT_LINE_STATUS_PORT(port)) & 0x20);
}

Serial::Serial(uint16_t port, uint16_t speed)
{
    this->port = port;
    this->speed = speed;
}

void Serial::initialize()
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

void Serial::write(char c)
{
    initialize();

    while(serialPortIsTransmitFIFOEmpty(port) == false);

    outport8(port, c);
}

void Serial::print(const char *string)
{
    initialize();

    while(*string)
    {
        write(*string);

        string++;
    }
}

void Serial::printLine(const char *string)
{
    print(string);
    print("\n");
}

void SerialPortOutStreamCOM1(char character, void *arg)
{
    SerialCOM1.write(character);
}
