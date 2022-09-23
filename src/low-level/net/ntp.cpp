#include <string.h>
#include "net/ntp.hpp"
#include "arpa/inet.h"
#include "net/dns.hpp"
#include "time.h"

int NTPRequest(NetworkDevice *device, const char *server, time_t *serverTime)
{
    /*
    NTPHeader header = {
        .livnmode = NTP_V4 | NTP_MODE_CLIENT,
    };

    header.originTimestamp.seconds = htonl(NTP_TIMESTAMP_DELTA + time(NULL));

    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(sockfd < 0)
    {
        DEBUG_OUT("[ntp] Failed to create socket", "");

        return NTP_ERR_SOCK;
    }

    in_addr address;
    int result = gethostbyname2(server, &address);

    if(result != 0)
    {
        DEBUG_OUT("[ntp] Failed to get host \"%s\" by name", server);

        return NTP_ERR_HOST;
    }

    sockaddr_in serverAddress = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_NTP),
        .sin_addr = address,
    };

    socklen_t serverAddressLength = sizeof(sockaddr_in);

    DEBUG_OUT("[ntp] Sending packet", 0);

    if(sendto(sockfd, &header, sizeof(NTPHeader), 0, (sockaddr *)&serverAddress, serverAddressLength) < 0)
    {
        close(sockfd);

        return NTP_ERR_SEND;
    }

    ssize_t bytesReceived = recvfrom(sockfd, &header, sizeof(NTPHeader), 0, (sockaddr *)&serverAddress, &serverAddressLength);

    close(sockfd);

    if(bytesReceived < (ssize_t)sizeof(NTPHeader))
    {
        DEBUG_OUT("[ntp] received less bytes than expected (%d instead of %d)", bytesReceived, sizeof(NTPHeader));

        return NTP_ERR_RECV;
    }

    header.transmitTimestamp.seconds = ntohl(header.transmitTimestamp.seconds);
    header.transmitTimestamp.fraction = ntohl((uint32_t)header.transmitTimestamp.fraction);

    *serverTime = header.transmitTimestamp.seconds - NTP_TIMESTAMP_DELTA;
    */

    return 0;
}