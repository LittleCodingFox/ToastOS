#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

typedef uint32_t in_addr_t;

struct in_addr
{
    in_addr_t s_addr;
};

struct sockaddr_in
{
    int16_t sin_family;
    uint16_t sin_port;
    struct in_addr sin_addr;
    uint8_t sin_zero[8];
};

in_addr_t inet_addr(const char *s);

in_addr_t inet_addr2(uint8_t ip[4]);

void inet_ntoi(uint32_t value, uint8_t *buffer, size_t length);

void inet_itoa(uint8_t ip[4], char *buffer, size_t length);

void inet_ntoa2(in_addr in, char *buffer, size_t length);

uint16_t htons(uint16_t value);

uint16_t ntohs(uint16_t value);

uint32_t htonl(uint32_t value);

uint32_t ntohl(uint32_t value);

#define HTONS(n) ((((n)&0xFF) << 8) | (((n)&0xFF00) >> 8))
#define NTOHS(n) HTONS(n)
#define HTONL(n)                                                               \
  ((((n)&0x000000FF) << 24) | (((n)&0x0000FF00) << 8) |                        \
   (((n)&0x00FF0000) >> 8) | (((n)&0xFF000000) >> 24))
#define NTOHL(n) HTONL(n)

#define htons(n) HTONS(n)
#define ntohs(n) NTOHS(n)
#define htonl(n) HTONL(n)
#define ntohl(n) NTOHL(n)
