#pragma once

#include <arpa/inet.h>
#include "kernel.h"
#include "ipv4.hpp"

#define PORT_DNS            53
#define PORT_DHCP_SERVER    67
#define PORT_DHCP_CLIENT    68
#define PORT_NTP            123

struct PACKED UDPHeader
{
    uint16_t sourcePort;
    uint16_t destinationPort;
    uint16_t length;
    uint16_t checksum;
};

void UDPReceivePacket(NetworkDevice *device, uint8_t *packet, IPV4Header *ipv4Header);

void UDPSendPacket(NetworkDevice *device, uint16_t sourcePort, uint8_t destinationMAC[6],
    sockaddr_in *destinationAddress,
    uint8_t *data,
    uint32_t length);
