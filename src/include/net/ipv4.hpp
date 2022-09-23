#pragma once

#include <arpa/inet.h>
#include "kernel.h"
#include "devicemanager/NetworkDevice.hpp"

#define IPV4_PROTO_ICMP     0x1
#define IPV4_PROTO_IGMP     0x2
#define IPV4_PROTO_TCP      0x6
#define IPV4_PROTO_UDP      0x11

#define IPV4_VERSION        4
#define IPV4_DEFAULT_TTL    255
#define IPV4_FLAG_DF        0x4000

struct PACKED IPV4Header
{
    uint8_t ihl : 4;
    uint8_t version : 4;
    uint8_t service;
    uint16_t length;
    uint16_t id;
    uint16_t flags;
    uint8_t ttl;
    uint8_t proto;
    uint16_t checksum;
    uint32_t sourceAddress;
    uint32_t destinationAddress;
};

void IPV4ReceivePacket(NetworkDevice *device, uint8_t *data, uint32_t length);

void IPV4SendPacket(NetworkDevice *device, sockaddr_in *destinationAddress, uint8_t protocol,
    uint16_t flags, uint8_t *data, uint32_t length);

uint16_t IPV4Checksum(void *address, int count);
