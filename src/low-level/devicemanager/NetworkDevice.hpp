#pragma once
#include "DeviceManager.hpp"

class NetworkDevice : public GenericDevice
{
public:
    virtual uint16_t NetworkType() = 0;
    virtual void ReceiveFrame(uint8_t *data, uint32_t length) = 0;
    virtual uint8_t *MAC() = 0;
    virtual uint8_t *IP() = 0;
    virtual uint8_t *GatewayMac() = 0;
    virtual uint8_t *GatewayIP() = 0;
    virtual uint8_t *DNSMac() = 0;
    virtual uint8_t *DNSIP() = 0;
    virtual void Transmit(uint8_t *data, uint32_t length) = 0;
};
