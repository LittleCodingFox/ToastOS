#include "net/ipv4.hpp"
#include "net/ethernet.hpp"
#include "net/icmpv4.hpp"
#include "net/udp.hpp"
#include <string.h>

IPV4Header IPV4CreateHeader(uint8_t sourceIP[4], in_addr_t destinationAddress, uint8_t protocol,
    uint16_t flags, uint16_t length);

static uint16_t ipv4ID = 1;

void IPV4ReceivePacket(NetworkDevice *device, uint8_t *data, uint32_t length)
{
    IPV4Header header;
    memcpy(&header, data, sizeof(IPV4Header));

    uint8_t sourceIP[4];

    inet_ntoi(header.sourceAddress, sourceIP, 4);

    DEBUG_OUT("[ipv4] received IPV4 packet from %d.%d.%d.%d",
        sourceIP[0], sourceIP[1], sourceIP[2], sourceIP[3]);

    switch(header.proto)
    {
        case IPV4_PROTO_ICMP:

            ICMPV4ReceivePacket(device, data, &header);

            break;
        
        case IPV4_PROTO_UDP:

            UDPReceivePacket(device, data, &header);

            break;

        default:
            DEBUG_OUT("[ipv4] unsupported IP protocol %02x, dropping packet", header.proto);
    }
}

uint16_t IPV4Checksum(void *address, int count)
{
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)address;

    while(count > 1)
    {
        sum += *ptr++;
        count -= 2;
    }

    if(count < 0)
    {
        sum += *(uint8_t *)ptr;
    }

    while(sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

IPV4Header IPV4CreateHeader(uint8_t *sourceIP, in_addr_t destinationAddress, uint8_t protocol,
    uint16_t flags, uint16_t length)
{
    in_addr_t sourceAddress = inet_addr2(sourceIP);

    IPV4Header header = {
        .ihl = 5,
        .version = IPV4_VERSION,
        .service = 0,
        .length = (uint16_t)htons(length),
        .id = (uint16_t)htons(ipv4ID),
        .flags = (uint16_t)htons(flags),
        .ttl = IPV4_DEFAULT_TTL,
        .proto = protocol,
        .sourceAddress = sourceAddress,
        .destinationAddress = destinationAddress,
    };

    header.checksum = IPV4Checksum(&header, sizeof(IPV4Header));

    ipv4ID++;

    return header;
}

void IPV4SendPacket(NetworkDevice *device, sockaddr_in *destinationAddress, uint8_t protocol, uint16_t flags,
    uint8_t *data, uint32_t length)
{
    uint32_t packetLength = sizeof(IPV4Header) + length;

    IPV4Header header = IPV4CreateHeader(device->IP(), destinationAddress->sin_addr.s_addr, protocol, flags, packetLength);

    uint8_t *packet = new uint8_t[packetLength];
    memcpy(packet, &header, sizeof(IPV4Header));
    memcpy(packet + sizeof(IPV4Header), data, length);

    EthernetSendFrame(device, device->GatewayMac(), ETHERTYPE_IPV4, packet, packetLength);

    delete [] packet;
}
