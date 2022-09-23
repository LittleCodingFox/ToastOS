#pragma once

#include "kernel.h"
#include "devicemanager/NetworkDevice.hpp"
#include "udp.hpp"

#define DNS_TYPE_A      0x0001
#define DNS_CLASS_IN    0x0001

#define DNS_ERR_SOCK        1
#define DNS_ERR_SEND        2
#define DNS_ERR_RECV        3
#define DNS_ERR_CLASS       4
#define DNS_ERR_NO_ANSWER   5

struct PACKED DNSHeader
{
    uint16_t id;
    uint16_t flags;
    uint16_t qdCount;
    uint16_t anCount;
    uint16_t nsCount;
    uint16_t arCount;
};

struct PACKED DNSAnswerHeader
{
    uint16_t name;
    uint16_t type;
    uint16_t class_;
    uint32_t ttl;
    uint16_t dataLength;
};

int DNSLookup(NetworkDevice *device, const char *domain, uint8_t ip[4]);
