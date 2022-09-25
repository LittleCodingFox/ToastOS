#pragma once

#include <stdint.h>

#define AF_INET         2

#define SOCK_DGRAM      2
#define SOCK_RAW        3

#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_UDP     17

typedef uint16_t sa_family_t;

typedef uint32_t socklen_t;

struct sockaddr
{
    sa_family_t sa_family;
    char sa_data[14];
};
