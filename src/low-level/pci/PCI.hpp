#pragma once

#include <stddef.h>
#include <stdint.h>
#include "acpi/ACPI.hpp"
#include <frg/optional.hpp>
#include <lai/core.h>
#include <lai/helpers/pci.h>

#define PCI_BAR_IO  1
#define PCI_BAR_MEM 2

enum class PCIBarType
{
    IO,
    MMIO
};

struct PCIBar
{
    PCIBarType type;
    uint64_t address;
    size_t length;
    bool prefetch;
};

enum class PCIRegister : uint16_t
{
    Vendor = 0x00,
    Device = 0x02,
    HeaderType = 0x0E,
    BaseClass = 0x0B,
    SubClass = 0x0A,
    ProgIf = 0x09,
    SecBus = 0x19,
    Bar0Offset = 0x10,
    IRQPin = 0x3D,
};

uint8_t PCIReadByte(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset);
uint8_t PCIReadByte(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg);

void PCIWriteByte(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint8_t value);
void PCIWriteByte(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg, uint8_t value);

uint16_t PCIReadWord(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset);
uint16_t PCIReadWord(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg);

void PCIWriteWord(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint16_t value);
void PCIWriteWord(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg, uint16_t value);

uint32_t PCIReadDword(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset);
uint32_t PCIReadDword(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg);

void PCIWriteDword(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint32_t value);
void PCIWriteDword(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg, uint32_t value);

class PCIBus;

class PCIDevice
{
private:
    PCIDevice(const PCIDevice &) = delete;
    PCIDevice &operator=(const PCIDevice &) = delete;
public:
    PCIDevice(uint8_t bus, uint8_t slot, uint8_t func);

    bool Exists();

    void Init();
    frg::optional<PCIBar> FetchBar(int bar);
    void AttachTo(PCIBus *bus);
    void RouteIRQ(lai_state_t *state);

    uint8_t Bus() const;
    uint8_t Slot() const;
    uint8_t Func() const;

    uint16_t Vendor() const;
    uint16_t Device() const;
    
    uint8_t BaseClass() const;
    uint8_t SubClass() const;
    uint8_t ProgIf() const;

    uint8_t HeaderType() const;
    bool MultiFunction() const;

    uint8_t SecBus() const;
    uint8_t IRQPin() const;

    uint8_t IRQ() const;
    uint8_t GSI() const;

    PCIBus *Parent();

private:
    PCIBus *parent;
    uint8_t bus;
    uint8_t slot;
    uint8_t func;

    uint16_t vendor;
    uint16_t device;

    uint8_t baseClass;
    uint8_t subClass;
    uint8_t progIf;

    uint8_t headerType;
    bool multiFunction;

    uint8_t secBus;
    uint8_t irqPin;
    uint8_t irq;
    uint8_t gsi;
};

class PCIBus
{
public:
    PCIBus(uint8_t bus, PCIDevice *device = nullptr);

    void SetACPINode(lai_nsnode *node);
    void FindNode(lai_state_t *state);

    lai_variable_t *PRT();
    void FetchPRT(lai_state_t *state);
    bool HasPRT() const;

    void Enumerate(lai_state_t *state);
    void AttachDevice(PCIDevice *device);
    void AttachTo(PCIBus *bus);

    PCIBus *Parent();
    PCIDevice *Device();
private:
    PCIBus *parent;

    PCIDevice *device;

    vector<PCIDevice *> children;

    uint8_t bus;

    bool hasPRT;
    bool triedPRTFetch;
    lai_nsnode_t *acpiNode;
    lai_variable_t acpiprt;
};

void InitializePCI();
void PCIEnumerateDevices(uint16_t vendor, uint16_t device, void (*callback)(PCIDevice *device));
void PCIEnumerateDevices(uint16_t vendor, uint16_t device, uint8_t classID, uint8_t subclassID, uint8_t progif, void (*callback)(PCIDevice *device));
