#include <arpa/inet.h>
#include "string/stringutils.hpp"

in_addr_t inet_addr(const char *s)
{
    uint8_t ip[4] = { 0 };

    auto pieces = SplitString(s, '.');

    if(pieces.size() != 4)
    {
        return inet_addr2(ip);
    }

    for(uint32_t i = 0; i < 4; i++)
    {
        ip[i] = atoi(pieces[i].data());
    }

    return inet_addr2(ip);
}

in_addr_t inet_addr2(uint8_t ip[4])
{
    return (in_addr_t)(ip[0] |
        ((uint32_t)ip[1] << 8) |
        ((uint32_t)ip[2] << 16) |
        ((uint32_t)ip[3] << 24));
}

void inet_itoa(uint8_t ip[4], char *buffer, size_t length)
{
    snprintf(buffer, length, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void inet_ntoa2(in_addr in, char *buffer, size_t length)
{
    uint8_t ip[4];
    inet_ntoi(in.s_addr, ip, 4);
    inet_itoa(ip, buffer, length);
}

void inet_ntoi(uint32_t value, uint8_t *buffer, size_t length)
{
    memset(buffer, 0, length);

    if(length >= 4)
    {
        for(uint32_t i = 0; i < 4; i++)
        {
            buffer[i] = ((uint8_t *)&value)[i];
        }
    }
}
