#pragma once

#include "arp.hpp"
#include "udp.hpp"

#define DHCP_MAGIC_COOKIE   0x63825363

#define DHCP_ROUTER         0x03
#define DHCP_DNS            0x06
#define DHCP_REQUESTED_IP   0x32
#define DHCP_MESSAGE_TYPE   0x35
#define DHCP_DHCP_SERVER    0x36

#define DHCP_DISCOVER       0x01
#define DHCP_OFFER          0x02
#define DHCP_REQUEST        0x03
#define DHCP_ACK            0x05

struct PACKED DHCPHeader
{
    uint8_t opcode;
    uint8_t htype;
    uint8_t hLength;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciAddress;
    uint32_t yiAddress;
    uint32_t siAddress;
    uint32_t giAddress;
    uint8_t chAddress[16];
    uint8_t zero[192];
    uint32_t magicCookie;
    uint8_t options[1024];
};

void DHCPNegotiate(NetworkDevice *device, void (*callback)(NetworkDevice *device, bool success));

void DHCPReceivePacket(NetworkDevice *device, uint8_t *packet, UDPHeader *header);
