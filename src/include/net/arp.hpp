#pragma once

#include <stdint.h>
#include "kernel.h"
#include "devicemanager/NetworkDevice.hpp"

#define ARP_HTYPE_ETHERNET  0x0001
#define ARP_REQUEST         0x0001
#define ARP_REPLY           0x0002

struct PACKED ARPPacket
{
    uint16_t hardwareType;
    uint16_t protocolType;
    uint8_t hardwareSize;
    uint8_t protocolSize;
    uint16_t opcode;
    uint8_t sourceMAC[6];
    uint8_t sourceIP[4];
    uint8_t destinationMAC[6];
    uint8_t destinationIP[4];
};

void ARPRequest(NetworkDevice *device, uint8_t ip[4]);

void ARPWaitReply(uint8_t *destinationMAC);

void ARPReceivePacket(NetworkDevice *device, uint8_t *data, uint32_t length);
