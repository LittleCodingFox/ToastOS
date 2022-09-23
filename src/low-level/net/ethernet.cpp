#include <arpa/inet.h>
#include "net/ethernet.hpp"
#include "net/arp.hpp"
#include "net/ipv4.hpp"
#include <string.h>

void EthernetReceiveFrame(NetworkDevice *device, uint8_t *data, uint32_t length)
{
    EthernetHeader frameHeader;

    memcpy(&frameHeader, data, sizeof(EthernetHeader));

    frameHeader.type = ntohs(frameHeader.type);

    DEBUG_OUT("[ethernet] received frame with type %x from %02x:%02x:%02x:%02x:%02x:%02x on interface %s",
        frameHeader.type,
        frameHeader.sourceMAC[0], frameHeader.sourceMAC[1], frameHeader.sourceMAC[2],
        frameHeader.sourceMAC[3], frameHeader.sourceMAC[4], frameHeader.sourceMAC[5],
        device->name());

    switch(frameHeader.type)
    {
        case ETHERTYPE_ARP:

            ARPReceivePacket(device, data + sizeof(EthernetHeader), length - sizeof(EthernetHeader));

            break;
        
        case ETHERTYPE_IPV4:

            IPV4ReceivePacket(device, data + sizeof(EthernetHeader), length - sizeof(EthernetHeader));

            break;
        
        default:
            DEBUG_OUT("[ethernet] unsupported ethernet frame: 0x%04x", frameHeader.type);

            break;
    }

    delete [] data;
}

void EthernetSendFrame(NetworkDevice *device, uint8_t destinationMAC[6], uint16_t type, uint8_t *data, uint32_t length)
{
    EthernetHeader header = {
        .type = (uint16_t)htons(type)
    };

    memcpy(header.sourceMAC, device->MAC(), 6);
    memcpy(header.destinationMAC, destinationMAC, 6);

    uint32_t frameLength = sizeof(EthernetHeader) + length;

    if(frameLength < 64)
    {
        frameLength = 64;
    }

    uint8_t *frame = (uint8_t *)calloc(1, frameLength);

    memcpy(frame, &header, sizeof(EthernetHeader));
    memcpy(frame + sizeof(EthernetHeader), data, length);

    device->Transmit(frame, frameLength);

    free(frame);
}
