#pragma once

#include "kernel.h"
#include "devicemanager/NetworkDevice.hpp"

#define ETHERTYPE_ARP   0x0806
#define ETHERTYPE_IPV4  0x0800

struct PACKED EthernetHeader
{
    uint8_t destinationMAC[6];
    uint8_t sourceMAC[6];
    uint16_t type;
};

void EthernetReceiveFrame(NetworkDevice *device, uint8_t *data, uint32_t length);

void EthernetSendFrame(NetworkDevice *device, uint8_t destinationMAC[6], uint16_t type, uint8_t *data, uint32_t length);
