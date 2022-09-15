#include "PCI.hpp"
#include "ports/Ports.hpp"
#include <frg/unique.hpp>

volatile MCFGHeader *mcfg = NULL;
volatile MADT *madt = NULL;
volatile SDTHeader *xsdt = NULL;

extern box<vector<frg::unique_ptr<PCIDevice, frg_allocator>>> devices;

uint8_t PCIReadByte(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset)
{
	outport32(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFF) | 0x80000000);

	uint8_t v = inport8(0xCFC + (offset % 4));

	return v;
}

uint8_t PCIReadByte(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg)
{
	return PCIReadByte(bus, slot, func, (uint16_t)reg);
}

void PCIWriteByte(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint8_t value)
{
	outport32(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFF) | 0x80000000);
	outport32(0xCFC + (offset % 4), value);
}

void PCIWriteByte(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg, uint8_t value)
{
	PCIWriteByte(bus, slot, func, (uint16_t)reg, value);
}

uint16_t PCIReadWord(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset)
{
	outport32(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFE) | 0x80000000);

	uint16_t v = inport16(0xCFC + (offset % 4));

	return v;
}

uint16_t PCIReadWord(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg)
{
	return PCIReadWord(bus, slot, func, (uint16_t)reg);
}

void PCIWriteWord(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint16_t value)
{
	outport32(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFE) | 0x80000000);
	outport16(0xCFC + (offset % 4), value);
}

void PCIWriteWord(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg, uint16_t value)
{
	PCIWriteWord(bus, slot, func, (uint16_t)reg, value);
}

uint32_t PCIReadDword(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset)
{
	outport32(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFC) | 0x80000000);

	uint32_t v = inport32(0xCFC + (offset % 4));

	return v;
}

uint32_t PCIReadDword(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg)
{
	return PCIReadDword(bus, slot, func, (uint16_t)reg);
}

void PCIWriteDword(uint32_t bus, uint32_t slot, uint32_t func, uint16_t offset, uint32_t value)
{
	outport32(0xCF8, (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFFFC) | 0x80000000);
	outport32(0xCFC + (offset % 4), value);
}

void PCIWriteDword(uint32_t bus, uint32_t slot, uint32_t func, PCIRegister reg, uint32_t value)
{
	PCIWriteDword(bus, slot, func, (uint16_t)reg, value);
}

void PCIEnumerateDevices(uint16_t vendor, uint16_t device, void (*callback)(PCIDevice *device))
{
	PCIEnumerateDevices(vendor, device, 0, 0, 0, callback);
}

void PCIEnumerateDevices(uint16_t vendor, uint16_t device, uint8_t classID, uint8_t subclassID, uint8_t progif, void (*callback)(PCIDevice *device))
{
	if(callback == nullptr)
	{
		return;
	}

	for(auto &d : *devices.get())
	{
		if(vendor != 0 && device != 0)
		{
			if(d->Vendor() == vendor && d->Device() == device)
			{
				callback(d.get());
			}
		}
		else
		{
			if(d->BaseClass() == classID && d->SubClass() == subclassID && d->ProgIf() == progif)
			{
				callback(d.get());
			}
		}
	}
}
