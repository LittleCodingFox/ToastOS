#pragma once

#include "kernel.h"
#include "ipv4.hpp"
#include "devicemanager/NetworkDevice.hpp"

#define ICMPV4_TYPE_REPLY       0
#define ICMPV4_TYPE_UNREACHABLE 3
#define ICMPV4_TYPE_REQUEST     8

struct PACKED ICMPV4Echo
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
    uint8_t *data;
};

struct ICMPV4Reply
{
    uint8_t sourceIP[4];
    uint16_t sequence;
    uint8_t ttl;
};

void ICMPV4ReceivePacket(NetworkDevice *device, uint8_t *packet, IPV4Header *header);

int ICMPV4SendPacket(NetworkDevice *device, uint8_t ip[4], ICMPV4Reply *reply);
