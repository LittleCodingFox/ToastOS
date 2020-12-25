#include "serial.h"
#include <low-level/ports/ports.h>
#include <stdbool.h>

#define SERIAL_PORT_DATA_PORT(base)             (base)
#define SERIAL_PORT_FIFO_COMMAND_PORT(base)     (base + 2)
#define SERIAL_PORT_LINE_COMMAND_PORT(base)     (base + 3)
#define SERIAL_PORT_MODEM_COMMAND_PORT(base)    (base + 4)
#define SERIAL_PORT_LINE_STATUS_PORT(base)      (base + 5)

#define SERIAL_PORT_LINE_ENABLE_DLAB            0x80

void serialPortInit(uint16_t port, uint16_t speed) {

    outport8(SERIAL_PORT_LINE_COMMAND_PORT(port), SERIAL_PORT_LINE_ENABLE_DLAB);
    outport8(SERIAL_PORT_DATA_PORT(port), (speed >> 8) & 0x00FF);
    outport8(SERIAL_PORT_DATA_PORT(port), speed & 0x00FF);

    outport8(SERIAL_PORT_LINE_COMMAND_PORT(port), 0x03);
    outport8(SERIAL_PORT_FIFO_COMMAND_PORT(port), 0xC7);
    outport8(SERIAL_PORT_MODEM_COMMAND_PORT(port), 0x08);
}

bool serialPortIsTransmitFIFOEmpty(uint16_t port) {

    return (inport8(SERIAL_PORT_LINE_STATUS_PORT(port)) & 0x20);
}

void serialPortWrite(uint16_t port, char c) {

    while(serialPortIsTransmitFIFOEmpty(port) == false);

    outport8(port, c);
}

void serialPortPrint(uint16_t port, const char *string) {

    while(*string) {

        serialPortWrite(port, *string);

        string++;
    }
}

void serialPortPrintLine(uint16_t port, const char *string) {

    serialPortPrint(port, string);
    serialPortPrint(port, "\n");
}
