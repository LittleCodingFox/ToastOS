#pragma once

#include <stddef.h>
#include <stdint.h>
#include "pci/PCI.hpp"
#include "devicemanager/NetworkDevice.hpp"
#include "interrupts/Interrupts.hpp"
#include "debug.hpp"

namespace Drivers
{
    namespace Networking
    {
        class RTL8139Driver : public NetworkDevice
        {
        private:
            PCIDevice *device;
            uint32_t IOAddress;
            uint8_t *TXBuffer;
            uint64_t TXBufferAddress;
            uint8_t *RXBuffer;
            uint32_t RXIndex;
            uint8_t MACAddress[6];
            uint8_t ip[4];
            uint8_t gatewayMAC[6];
            uint8_t gatewayIP[4];
            uint8_t DNSMAC[6];
            uint8_t dnsIP[4];
            uint8_t currentTransmitPair;
        public:
            RTL8139Driver(PCIDevice *device);
            void Callback(InterruptStack *stack);
            virtual const char *name() const override;
            virtual DeviceType type() const override;
            virtual uint16_t NetworkType() override;
            virtual void ReceiveFrame(uint8_t *data, uint32_t length) override;
            virtual uint8_t *MAC() override;
            virtual uint8_t *IP() override;
            virtual uint8_t *GatewayMac() override;
            virtual uint8_t *GatewayIP() override;
            virtual uint8_t *DNSMac() override;
            virtual uint8_t *DNSIP() override;
            virtual void Transmit(uint8_t *data, uint32_t length) override;
        };

        void RTL8139Query();
    }
}
