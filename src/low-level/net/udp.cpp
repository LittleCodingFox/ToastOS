#include "net/udp.hpp"
#include "arpa/inet.h"
#include "net/dhcp.hpp"
#include "net/dns.hpp"
#include <string.h>

void UDPReceivePacket(NetworkDevice *device, uint8_t *packet, IPV4Header *IPHeader)
{
    uint8_t *IPData = packet + (4 * IPHeader->ihl);

    UDPHeader header;

    memcpy(&header, IPData, sizeof(UDPHeader));

    header.sourcePort = ntohs(header.sourcePort);
    header.destinationPort = ntohs(header.destinationPort);
    header.length = ntohs(header.length);
    header.checksum = ntohs(header.checksum);

    DEBUG_OUT("[udp] packet received: source port: %d, destination port: %d, length: %d, checksum: %x",
        header.sourcePort, header.destinationPort, header.length, header.checksum);
    
    uint8_t *UDPData = IPData + sizeof(UDPHeader);

    //TODO: FD

    switch(header.destinationPort)
    {
        case PORT_DHCP_CLIENT:
            DHCPReceivePacket(device, UDPData, &header);

            break;
        
        default:
            DEBUG_OUT("[udp] Dropping packet (source port: %d, destination port: %d) because it cannot be handled",
                header.sourcePort, header.destinationPort);
    }
}

void UDPSendPacket(NetworkDevice *device, uint16_t sourcePort, uint8_t destinationMAC[6], sockaddr_in *destinationAddress,
    uint8_t *data, uint32_t length)
{
    uint16_t UDPLength = sizeof(UDPHeader) + length;

    UDPHeader header = {
        .sourcePort = (uint16_t)htons(sourcePort),
        .destinationPort = destinationAddress->sin_port,
        .length = (uint16_t)htons(UDPLength),
        .checksum = 0
    };

    uint32_t pseudoHeaderLength = 9 + (length / 2);

    if(length & 1)
    {
        pseudoHeaderLength++;
    }

    uint16_t *pseudoHeader = new uint16_t[pseudoHeaderLength];
    memset(pseudoHeader, 0, sizeof(uint16_t[pseudoHeaderLength]));

    pseudoHeader[0] = device->IP()[0] | (device->IP()[1] << 8);
    pseudoHeader[1] = device->IP()[2] | (device->IP()[3] << 8);
    pseudoHeader[2] = (uint16_t)(destinationAddress->sin_addr.s_addr >> 16);
    pseudoHeader[3] = (uint16_t)destinationAddress->sin_addr.s_addr;
    pseudoHeader[4] = htons(IPV4_PROTO_UDP);
    pseudoHeader[5] = htons(length + sizeof(UDPHeader));
    pseudoHeader[6] = htons(sourcePort);
    pseudoHeader[7] = destinationAddress->sin_port;
    pseudoHeader[8] = htons(length + sizeof(UDPHeader));

    uint32_t index = 9;

    for(uint32_t i = 0; i < length; i += 2)
    {
        if(i + 1 < length)
        {
            pseudoHeader[index++] = data[i] | (data[i + 1] << 8);
        }
        else
        {
            pseudoHeader[index++] = data[i] | (0x0 << 8);
        }
    }

    header.checksum = IPV4Checksum(pseudoHeader, pseudoHeaderLength);

    delete [] pseudoHeader;

    uint8_t *datagram = new uint8_t[UDPLength];
    memcpy(datagram, &header, sizeof(UDPHeader));
    memcpy(datagram + sizeof(UDPHeader), data, length);

    DEBUG_OUT("[udp] sending packet to destination port %d", ntohs(destinationAddress->sin_port));

    IPV4SendPacket(device, destinationAddress, IPV4_PROTO_UDP, IPV4_FLAG_DF, datagram, UDPLength);

    delete [] datagram;
}
