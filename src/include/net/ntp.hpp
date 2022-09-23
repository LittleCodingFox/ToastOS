#pragma once

#include "kernel.h"
#include "udp.hpp"
#include "time.h"
#include "devicemanager/NetworkDevice.hpp"

#define NTP_MODE_CLIENT     3
#define NTP_V4              (4 << 3)

#define NTP_ERR_SOCK        1
#define NTP_ERR_SEND        2
#define NTP_ERR_RECV        3
#define NTP_ERR_HOST        4

#define NTP_TIMESTAMP_DELTA 2208988800ull

struct PACKED NTPTimestamp
{
    uint32_t seconds;
    int32_t fraction;
};

struct PACKED NTPHeader
{
    uint8_t livnmode;
    uint8_t stratum;
    int8_t poll;
    int8_t precision;
    int32_t rootDelay;
    int32_t rootDispersion;
    uint32_t referenceID;
    NTPTimestamp referenceTimestamp;
    NTPTimestamp originTimestamp;
    NTPTimestamp receiveTimestamp;
    NTPTimestamp transmitTimestamp;
};

int NTPRequest(NetworkDevice *device, const char *server, time_t *serverTime);
