#include "net/dhcp.hpp"
#include "devicemanager/NetworkDevice.hpp"
#include "cmos/cmos.hpp"
#include <arpa/inet.h>
#include <string.h>

void DHCPDiscover(NetworkDevice *device);
void DHCPHandleOffer(NetworkDevice *device);
void DHCPReadOption(uint8_t *options, uint8_t code, uint8_t *buffer, uint16_t length);

static uint8_t broadcastMAC[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };
static uint8_t broadcastIP[4] = { 255, 255, 255, 255 };
static uint32_t xid = 1;

static DHCPHeader offer = {0};
static bool hasOffer = false;

void DHCPNegotiate(NetworkDevice *device, void (*callback)(NetworkDevice *device, bool success))
{
    DHCPDiscover(device);

    uint64_t elapsed = cmos.CurrentTime();

    while(cmos.CurrentTime() - elapsed < 10 && hasOffer == false) {}

    if(hasOffer == false)
    {
        callback(device, false);

        return;
    }

    DHCPHandleOffer(device);

    elapsed = cmos.CurrentTime();

    while(cmos.CurrentTime() - elapsed < 10 && hasOffer == false) {}

    if(hasOffer == false)
    {
        callback(device, false);

        return;
    }

    DHCPHandleOffer(device);

    hasOffer = false;

    DEBUG_OUT("[dhcp] Finalized negotiation: device has IP %d.%d.%d.%d, gateway %d.%d.%d.%d, dns %d.%d.%d.%d",
        device->IP()[0], device->IP()[1], device->IP()[2], device->IP()[3],
        device->GatewayIP()[0], device->GatewayIP()[1], device->GatewayIP()[2], device->GatewayIP()[3],
        device->DNSIP()[0], device->DNSIP()[1], device->DNSIP()[2], device->DNSIP()[3]);

    callback(device, true);
}

void DHCPDiscover(NetworkDevice *device)
{
    xid++;

    DHCPHeader header = {
        .opcode = DHCP_DISCOVER,
        .htype = (uint8_t)device->NetworkType(),
        .hLength = 0x06,
        .hops = 0,
        .xid = htonl(xid),
        .magicCookie = htonl(DHCP_MAGIC_COOKIE),
        .options = { DHCP_MESSAGE_TYPE, 0x01, DHCP_DISCOVER, 0xFF },
    };

    memcpy(header.chAddress, device->MAC(), 6);

    sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_DHCP_SERVER),
        .sin_addr = { .s_addr = inet_addr2(broadcastIP) },
    };

    uint16_t length = sizeof(DHCPHeader);
    uint8_t *packet = new uint8_t[length];
    memcpy(packet, &header, length);

    UDPSendPacket(device, PORT_DHCP_CLIENT, broadcastMAC, &address, packet, length);

    delete [] packet;
}

void DHCPReceivePacket(NetworkDevice *device, uint8_t *packet, UDPHeader *header)
{
    DHCPHeader dhcpHeader = { 0 };

    memcpy(&dhcpHeader, packet, sizeof(DHCPHeader));

    dhcpHeader.xid = ntohl(dhcpHeader.xid);

    DEBUG_OUT("[dhcp] packet received - opcode=0x%02x xid=0x%08x", dhcpHeader.opcode, dhcpHeader.xid);

    if(dhcpHeader.xid != xid)
    {
        DEBUG_OUT("[dhcp] xid mismatch, dropping packet", 0);

        return;
    }

    switch(dhcpHeader.opcode)
    {
        case DHCP_OFFER:

            offer = dhcpHeader;

            hasOffer = true;

            break;

        default:

            DEBUG_OUT("[dhcp] unsupported DHCP opcode=0x%02x", dhcpHeader.opcode);

            break;
    }
}

void DHCPHandleOffer(NetworkDevice *device)
{
    hasOffer = false;

    uint8_t yiAddress[4];
    uint8_t siAddress[4];
    uint8_t type;

    inet_ntoi(offer.yiAddress, yiAddress, 4);
    inet_ntoi(offer.siAddress, siAddress, 4);
    DHCPReadOption(offer.options, DHCP_MESSAGE_TYPE, &type, 1);

    DEBUG_OUT("[dhcp] Received offer - type=0x%02x yiAddress=%d.%d.%d.%d siAddress=%d.%d.%d.%d",
        type,
        yiAddress[0], yiAddress[1], yiAddress[2], yiAddress[3],
        siAddress[0], siAddress[1], siAddress[2], siAddress[3]);

    if(type == DHCP_OFFER)
    {
        DHCPHeader request = {
            .opcode = DHCP_DISCOVER,
            .htype = (uint8_t)device->NetworkType(),
            .hLength = 0x06,
            .hops = 0,
            .xid = htonl(xid),
            .ciAddress = offer.yiAddress,
            .siAddress = offer.siAddress,
            .magicCookie = htonl(DHCP_MAGIC_COOKIE),
            .options = {
                DHCP_MESSAGE_TYPE,
                0x01,
                DHCP_REQUEST,
                DHCP_REQUESTED_IP,
                0x04,
                yiAddress[0],
                yiAddress[1],
                yiAddress[2],
                yiAddress[3],
                DHCP_DHCP_SERVER,
                0x04,
                siAddress[0],
                siAddress[1],
                siAddress[2],
                siAddress[3],
                0xFF
            },
        };

        memcpy(request.chAddress, device->MAC(), 6);

        sockaddr_in address = {
            .sin_family = AF_INET,
            .sin_port = htons(PORT_DHCP_SERVER),
            .sin_addr = { .s_addr = inet_addr2(broadcastIP) },
        };

        uint16_t length = sizeof(DHCPHeader);
        uint8_t *packet = new uint8_t[length];
        memcpy(packet, &request, length);

        UDPSendPacket(device, PORT_DHCP_CLIENT, broadcastMAC, &address, packet, length);

        delete [] packet;
    }
    else if(type == DHCP_ACK)
    {
        memcpy(device->IP(), yiAddress, 4);

        DHCPReadOption(offer.options, DHCP_ROUTER, device->GatewayIP(), 4);

        DHCPReadOption(offer.options, DHCP_DNS, device->DNSIP(), 4);
    }
    else
    {
        DEBUG_OUT("[dhcp] Unsupported dhcp message type %x", type);
    }
}

void DHCPReadOption(uint8_t *options, uint8_t code, uint8_t *buffer, uint16_t length)
{
    for(uint32_t i = 0; i < 1024; i++)
    {
        if(options[i] == code && options[i + 1] == length)
        {
            memcpy(buffer, options + i + 2, length);

            return;
        }
    }
}
