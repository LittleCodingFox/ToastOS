#include <string.h>
#include <sys/types.h>
#include <stddef.h>
#include "net/dns.hpp"
#include "arpa/inet.h"
#include "process/Process.hpp"

static uint16_t dnsID = 1;

int DNSLookup(NetworkDevice *device, const char *domain, uint8_t ip[4])
{
    auto process = processManager->CurrentProcess();

    if(process == nullptr || process->isValid == false)
    {
        return DNS_ERR_NO_ANSWER;
    }

    memset(ip, 0, 4);

    DNSHeader lookupHeader = {
        .id = (uint16_t)htons(dnsID),
        .flags = htons(0x0100),
        .qdCount = htons(0x0001),
    };

    uint32_t domainLength = strlen(domain);
    uint32_t dataLength = 6 + domainLength;

    uint8_t *data = new uint8_t[dataLength];

    char *_domain = (char *)calloc(1, domainLength + 1);
    strcat(_domain, domain);
    strcat(_domain, ".");

    uint32_t j = 0, position = 0;

    for(uint32_t i = 0; i < domainLength + 1; i++)
    {
        if(_domain[i] == '.')
        {
            data[j++] = i - position;

            while(position < i)
            {
                data[j++] = _domain[position++];
            }

            position++;
        }
    }

    data[j++] = '\0';
    data[j++] = (uint8_t)(DNS_TYPE_A << 8);
    data[j++] = (uint8_t)(DNS_TYPE_A);
    data[j++] = (uint8_t)(DNS_CLASS_IN << 8);
    data[j++] = (uint8_t)(DNS_CLASS_IN);

    free(_domain);

    uint16_t length = sizeof(DNSHeader) + dataLength;
    uint8_t *packet = new uint8_t[length];

    memcpy(packet, &lookupHeader, sizeof(DNSHeader));
    memcpy(packet + sizeof(DNSHeader), data, dataLength);

    delete [] data;

    auto socket = new ProcessFDSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, PORT_DNS);

    int sockfd = process->info->AddFD(ProcessFDType::Socket, socket);

    if(sockfd < 0)
    {
        DEBUG_OUT("Failed to create a socket (sockfd = %d)", sockfd);

        return DNS_ERR_SOCK;
    }

    sockaddr_in serverAddress = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_DNS),
        .sin_addr = { .s_addr = inet_addr2(device->DNSIP()) },
    };

    DEBUG_OUT("[dns] sending packet: id=0x%05x, flags=0x%04x", ntohs(lookupHeader.id), ntohs(lookupHeader.flags));

    UDPSendPacket(device, PORT_DNS, &serverAddress, packet, length);

    delete [] packet;

    dnsID++;

    DEBUG_OUT("[dns] waiting for a response now (sockfd=%d)", sockfd);

    uint8_t buffer[128];
    int error = 0;

    ssize_t receivedBytes = socket->Read(buffer, sizeof(buffer), &error);

    process->info->CloseFD(sockfd);

    if(receivedBytes < (ssize_t)sizeof(DNSHeader))
    {
        DEBUG_OUT("[dns] didn't receive enough data: %d", receivedBytes);

        return DNS_ERR_RECV;
    }

    DNSHeader header;
    memcpy(&header, buffer, sizeof(DNSHeader));

    header.id = ntohs(header.id);
    header.flags = ntohs(header.flags);
    header.qdCount = ntohs(header.qdCount);
    header.anCount = ntohs(header.anCount);
    header.nsCount = ntohs(header.nsCount);
    header.arCount = ntohs(header.arCount);

    DEBUG_OUT("dns: packet received: id=0x%04x, qdCount=%d, anCount=%d, bytes=%d",
        header.id, header.qdCount, header.anCount, receivedBytes);

    uint8_t *dnsData = buffer + sizeof(DNSHeader);

    uint16_t queryLength = 0;

    for(uint16_t query = 0; query < header.qdCount; query++)
    {
        while(dnsData[queryLength++] != 0x00);

        queryLength += 4;
    }

    if(header.anCount > 0)
    {
        DNSAnswerHeader answerHeader;
        memcpy(&answerHeader, dnsData + queryLength, sizeof(DNSAnswerHeader));

        answerHeader.name = ntohs(answerHeader.name);
        answerHeader.type = ntohs(answerHeader.type);
        answerHeader.class_ = ntohs(answerHeader.class_);
        answerHeader.ttl = ntohl(answerHeader.ttl);
        answerHeader.dataLength = ntohs(answerHeader.dataLength);

        if(answerHeader.class_ == DNS_CLASS_IN && answerHeader.dataLength == 0)
        {
            memcpy(ip, dnsData + queryLength + sizeof(DNSAnswerHeader), answerHeader.dataLength);
        }
        else
        {
            DEBUG_OUT("[dns] wrong dns class", 0);

            return DNS_ERR_CLASS;
        }
    }
    else
    {
        DEBUG_OUT("[dns] no answer", 0);

        return DNS_ERR_NO_ANSWER;
    }

    return 0;
}
