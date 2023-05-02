#include "RTL8139.hpp"
#include "kernel.h"
#include "ports/Ports.hpp"
#include "mmio/MMIO.hpp"
#include "interrupts/IDT.hpp"
#include "paging/PageTableManager.hpp"

#define RTL8139_VENDOR_ID 0x10ec
#define RTL8139_DEVICE_ID 0x8139

#define RTL8139_REGISTER_RBSTART  0x30
#define RTL8139_REGISTER_COMMAND  0x37
#define RTL8139_REGISTER_RX_PTR   0x38
#define RTL8139_REGISTER_IMR      0x3C
#define RTL8139_REGISTER_ISR      0x3E
#define RTL8139_REGISTER_CONFIG_1 0x52
#define RTL8139_REGISTER_MAC1     0x00
#define RTL8139_REGISTER_MAC2     0x04

// Receive OK.
#define RTL8139_ISR_ROK (1 << 0)
// Transmit OK.
#define RTL8139_ISR_TOK (1 << 2)

#define RX_BUFFER_SIZE (8192 + 16 + 1500)
#define TX_BUFFER_SIZE 1536

static uint8_t transmitStartRegisters[4] = { 0x20, 0x24, 0x28, 0x2C };
static uint8_t transmitCommandRegisters[4] = { 0x10, 0x14, 0x18, 0x1C };

namespace Drivers
{
    namespace Networking
    {
        void RTLProxy(InterruptStack *stack, void *arg);
        
        RTL8139Driver::RTL8139Driver(PCIDevice *device) : device(device), IOAddress(0), TXBuffer(0), TXBufferAddress(0),
            RXBuffer(0), RXIndex(0), MACAddress{0}, currentTransmitPair(0)
        {
            uint32_t result = PCIReadDword(device->Bus(), device->Slot(), device->Func(), PCIRegister::Bar0Offset);
            IOAddress = result & (~0x3);

            uint16_t commandReg = PCIReadWord(device->Bus(), device->Slot(), device->Func(), PCIRegister::Command);

            if((commandReg & (1 << 2)) == false)
            {
                commandReg |= (1 << 2);

                PCIWriteWord(device->Bus(), device->Slot(), device->Func(), PCIRegister::Command, commandReg);
            }

            outport8(IOAddress + RTL8139_REGISTER_CONFIG_1, 0x0);

            outport8(IOAddress + RTL8139_REGISTER_COMMAND, 0x10);

            while((inport8(IOAddress + RTL8139_REGISTER_COMMAND) & 0x10) != 0);

            TXBuffer = (uint8_t *)calloc(1, TX_BUFFER_SIZE);
            TXBufferAddress = TranslateToPhysicalMemoryAddress((uint64_t)TXBuffer);

            RXBuffer = (uint8_t *)calloc(1, RX_BUFFER_SIZE);

            outport32(IOAddress + RTL8139_REGISTER_RBSTART, TranslateToPhysicalMemoryAddress((uint64_t)RXBuffer));

            outport16(IOAddress + RTL8139_REGISTER_IMR, 0x0005);

            outport32(IOAddress + 0x44, 0xF | (1 << 7));

            outport8(IOAddress + RTL8139_REGISTER_COMMAND, 0x0C);

            uint8_t IRQ = PCIReadByte(device->Bus(), device->Slot(), device->Func(), PCIRegister::IRQLine);

            if(idt.ReserveVector(IRQ + 32))
            {
                DEBUG_OUT("[net] RTL8139: Registered IRQ %u", IRQ);

                interrupts.RegisterHandler(IRQ + 32, RTLProxy, this);
            }
            else
            {
                DEBUG_OUT("%s", "[net] RTL8139: Failed to reserve IRQ for RTL8139");
            }

            uint32_t mac1 = inport32(IOAddress + RTL8139_REGISTER_MAC1);
            uint16_t mac2 = inport16(IOAddress + RTL8139_REGISTER_MAC2);

            MACAddress[0] = mac1;
            MACAddress[1] = mac1 >> 8;
            MACAddress[2] = mac1 >> 16;
            MACAddress[3] = mac1 >> 24;
            MACAddress[4] = mac2;
            MACAddress[5] = mac2 >> 8;

            DEBUG_OUT("[net] RTL8139: MAC Address is %02x:%02x:%02x:%02x:%02x:%02x",
                MACAddress[0],
                MACAddress[1],
                MACAddress[2],
                MACAddress[3],
                MACAddress[4],
                MACAddress[5]);
        }

        void RTL8139Driver::Callback(InterruptStack *stack)
        {
            DEBUG_OUT("%s", "[net] RTL8139: interrupt received");

            for(;;)
            {
                uint16_t status = inport16(IOAddress + RTL8139_REGISTER_ISR);

                DEBUG_OUT("[net] RTL8139: status: %u", status);

                outport16(IOAddress + RTL8139_REGISTER_ISR, status);

                if(status == 0)
                {
                    break;
                }

                if(status & RTL8139_ISR_TOK)
                {
                    DEBUG_OUT("%s", "[net] RTL8139: frame transmitted");
                }

                if(status & RTL8139_ISR_ROK)
                {
                    DEBUG_OUT("%s", "[net] RTL8139: frame received");

                    uint8_t *buffer = RXBuffer;
                    uint32_t index = RXIndex;

                    while((inport8(IOAddress + RTL8139_REGISTER_COMMAND) & 0x01) == 0)
                    {
                        uint32_t offset = index % RX_BUFFER_SIZE;

                        uint32_t length = (buffer[3 + index] << 8) + buffer[2 + index];

                        uint8_t *frame = new uint8_t[length];

                        memcpy(frame, &buffer[offset + 4], length);

                        ReceiveFrame(frame, length);

                        delete [] frame;

                        index = (index + length + 4 + 3) & ~3;

                        outport16(IOAddress + RTL8139_REGISTER_RX_PTR, index - 16);
                    }

                    RXIndex = index;
                }
            }
        }

        const char *RTL8139Driver::name() const
        {
            return "RealTek RTL8139";
        }

        DeviceType RTL8139Driver::type() const
        {
            return DEVICE_TYPE_NETWORK;
        }

        uint16_t RTL8139Driver::NetworkType()
        {
            //TODO
            return 0; //ARP_HTYPE_ETHERNET;
        }

        void RTL8139Driver::ReceiveFrame(uint8_t *data, uint32_t length)
        {
            //TODO
            //EthernetReceiveFrame(this, data, length);
        }

        uint8_t *RTL8139Driver::MAC()
        {
            return MACAddress;
        }

        uint8_t *RTL8139Driver::IP()
        {
            return ip;
        }

        uint8_t *RTL8139Driver::GatewayMac()
        {
            return gatewayMAC;
        }

        uint8_t *RTL8139Driver::GatewayIP()
        {
            return gatewayIP;
        }

        uint8_t *RTL8139Driver::DNSMac()
        {
            return DNSMAC;
        }

        uint8_t *RTL8139Driver::DNSIP()
        {
            return dnsIP;
        }

        void RTL8139Driver::Transmit(uint8_t *data, uint32_t length)
        {
            memcpy(TXBuffer, data, length);

            outport32(IOAddress + transmitStartRegisters[currentTransmitPair], TXBufferAddress);
            outport32(IOAddress + transmitCommandRegisters[currentTransmitPair], length);

            currentTransmitPair++;

            if(currentTransmitPair > 3)
            {
                currentTransmitPair = 0;
            }
        }

        void RTL8139Query()
        {
            PCIEnumerateDevices(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, [] (PCIDevice *device) {

                RTL8139Driver *driver = new RTL8139Driver(device);

                globalDeviceManager.AddDevice(driver);
            });
        }

        void RTLProxy(InterruptStack *stack, void *arg)
        {
            auto driver = (RTL8139Driver *)arg;

            driver->Callback(stack);
        }
    }
}
