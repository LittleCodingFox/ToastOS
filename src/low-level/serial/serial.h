#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#define SERIAL_PORT_COM1            0x3F8

#define SERIAL_PORT_SPEED_115200    1
#define SERIAL_PORT_SPEED_57600     2
#define SERIAL_PORT_SPEED_38400     3

void serialPortInit(uint16_t port, uint16_t speed);

void serialPortPrint(uint16_t port, const char *string);

void serialPortPrintLine(uint16_t port, const char *string);

#endif
