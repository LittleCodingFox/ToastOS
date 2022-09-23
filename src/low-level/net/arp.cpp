#include <arpa/inet.h>
#include <string.h>
#include "net/arp.hpp"
#include "net/ethernet.hpp"
#include "devicemanager/NetworkDevice.hpp"
#include "cmos/cmos.hpp"

void ARPReply(NetworkDevice *device, ARPPacket *request);

static uint8_t lastARPMACAddress[6] = {0};
static bool lastARPMACAddressSet = false;

void ARPRequest(NetworkDevice *device, uint8_t *ip)
{
    uint8_t destinationMAC[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };

    ARPPacket packet = {
        .hardwareType = (uint16_t)htons(device->NetworkType()),
        .protocolType = htons(ETHERTYPE_IPV4),
        .hardwareSize = 6,
        .protocolSize = 4,
        .opcode = htons(ARP_REQUEST),
    };

    memcpy(packet.sourceMAC, device->MAC(), 6);
    memcpy(packet.sourceIP, device->IP(), 4);
    memcpy(packet.destinationMAC, destinationMAC, 6);
    memcpy(packet.destinationIP, ip, 4);

    uint8_t length = sizeof(ARPPacket);
    uint8_t *outPacket = new uint8_t[length];
    memcpy(outPacket, &packet, length);

    EthernetSendFrame(device, destinationMAC, ETHERTYPE_ARP, outPacket, length);

    delete [] outPacket;

    DEBUG_OUT("[arp] Requesting data on IP %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void ARPWaitReply(uint8_t *destinationMAC)
{
    memset(destinationMAC, 0, 6);

    uint64_t time = cmos.CurrentTime();

    while(cmos.CurrentTime() - time < 10) {}

    if(lastARPMACAddressSet)
    {
        memcpy(destinationMAC, lastARPMACAddress, 6);

        lastARPMACAddressSet = false;

        return;
    }

    DEBUG_OUT("[arp] no reply", 0);
}

void ARPReceivePacket(NetworkDevice *device, uint8_t *data, uint32_t length)
{
    ARPPacket packet;

    memcpy(&packet, data, sizeof(ARPPacket));

    packet.hardwareType = (uint16_t)ntohs(packet.hardwareType);
    packet.protocolType = (uint16_t)ntohs(packet.protocolType);
    packet.opcode = ntohs(packet.opcode);

    DEBUG_OUT("[arp] received packet from %d.%d.%d.%d (%02x:%02x:%02x:%02x:%02x:%02x) on interface %s",
        packet.sourceIP[0], packet.sourceIP[1], packet.sourceIP[2], packet.sourceIP[3],
        packet.sourceMAC[0], packet.sourceMAC[1], packet.sourceMAC[2], packet.sourceMAC[3],
        packet.sourceMAC[4], packet.sourceMAC[5],
        device->name());

    switch(packet.opcode)
    {
        case ARP_REQUEST:

            DEBUG_OUT("[arp] packet is a request", 0);

            if(packet.destinationIP[0] == device->IP()[0] &&
                packet.destinationIP[1] == device->IP()[1] &&
                packet.destinationIP[2] == device->IP()[2] &&
                packet.destinationIP[3] == device->IP()[3])
            {
                ARPReply(device, &packet);
            }

            break;

        case ARP_REPLY:

            DEBUG_OUT("[arp] packet is a reply", 0);

            memcpy(lastARPMACAddress, packet.sourceMAC, 6);
            lastARPMACAddressSet = true;

            break;
    }
}

void ARPReply(NetworkDevice *device, ARPPacket *request)
{
    ARPPacket reply = {
        .hardwareType = (uint16_t)htons(request->hardwareType),
        .protocolType = (uint16_t)htons(request->protocolType),
        .hardwareSize = 6,
        .protocolSize = 4,
        .opcode = htons(ARP_REPLY),
    };

    memcpy(reply.sourceMAC, device->MAC(), 6);
    memcpy(reply.sourceIP, device->IP(), 4);
    memcpy(reply.destinationMAC, request->sourceMAC, 6);
    memcpy(reply.destinationIP, request->sourceIP, 4);

    uint8_t length = sizeof(ARPPacket);
    uint8_t *packet = new uint8_t[length];

    memcpy(packet, &reply, length);

    EthernetSendFrame(device, request->sourceMAC, ETHERTYPE_ARP, packet, length);

    delete [] packet;
}
