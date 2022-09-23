#include <string.h>
#include "net/icmpv4.hpp"

static uint16_t icmpv4ID = 1;
static ICMPV4Reply *reply = nullptr;

void ICMPV4ReceivePacket(NetworkDevice *device, uint8_t *data, IPV4Header *header)
{
    ICMPV4Echo echo;

    memcpy(&echo, data + 4 * header->ihl, sizeof(ICMPV4Echo));

    DEBUG_OUT("[icmpv4] received packet type=%d code=%d checksum=%x", echo.type, echo.code, echo.checksum);

    if(echo.type == ICMPV4_TYPE_REPLY)
    {
        reply = new ICMPV4Reply();

        reply->sequence = echo.sequence;
        reply->ttl = header->ttl;

        inet_ntoi(header->sourceAddress, reply->sourceIP, 4);
    }
}

int ICMPV4SendPacket(NetworkDevice *device, uint8_t ip[4], ICMPV4Reply *reply)
{
    ICMPV4Echo echo = {
        .type = ICMPV4_TYPE_REQUEST,
        .code = 0,
        .checksum = 0,
        .id = (uint16_t)htons(icmpv4ID),
        .sequence = 0,
        .data = 0,
    };

    echo.checksum = IPV4Checksum(&echo, sizeof(ICMPV4Echo));

    DEBUG_OUT("[icmpv4] sending packet id=0x%04x", ntohs(echo.id));

    sockaddr_in destinationAddress = {
        .sin_addr = { .s_addr = inet_addr2(ip) }
    };

    IPV4SendPacket(device, &destinationAddress, IPV4_PROTO_ICMP, 0, (uint8_t *)&echo, sizeof(ICMPV4Echo));

    uint64_t elapsed = 0;

    while(::reply == nullptr && elapsed < 10000);

    if(::reply == nullptr)
    {
        return -1;
    }

    memcpy(reply, ::reply, sizeof(ICMPV4Reply));

    delete ::reply;

    ::reply = nullptr;

    return 0;
}
