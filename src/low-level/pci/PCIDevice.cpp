#include "PCI.hpp"
#include "ports/Ports.hpp"

PCIDevice::PCIDevice(uint8_t bus, uint8_t slot, uint8_t func) : parent(nullptr), bus(bus), slot(slot), func(func) {}

bool PCIDevice::Exists()
{
    return PCIReadWord(bus, slot, func, PCIRegister::Vendor) != 0xFFFF;
}

void PCIDevice::Init()
{
    if(Exists() == false)
    {
        return;
    }

    vendor = PCIReadWord(bus, slot, func, PCIRegister::Vendor);
    device = PCIReadWord(bus, slot, func, PCIRegister::Device);
    baseClass = PCIReadByte(bus, slot, func, PCIRegister::BaseClass);
    subClass = PCIReadByte(bus, slot, func, PCIRegister::SubClass);
    progIf = PCIReadByte(bus, slot, func, PCIRegister::ProgIf);
    headerType = PCIReadByte(bus, slot, func, PCIRegister::HeaderType);
    irqPin = PCIReadByte(bus, slot, func, PCIRegister::IRQPin);
    secBus = PCIReadByte(bus, slot, func, PCIRegister::SecBus);

    irq = 0;
    gsi = 0;
    multiFunction = headerType & 0x80;
}

frg::optional<PCIBar> PCIDevice::FetchBar(int bar)
{
    if(headerType & 0x7F)
    {
        return frg::null_opt;
    }

    uint16_t barOffset = (uint16_t)PCIRegister::Bar0Offset + bar * 4;

    uint64_t barHigh = 0;
    uint64_t barLow;
    uint64_t barSizeHigh;
    uint64_t barSizeLow;
    uint64_t base;
    size_t size;

    barLow = PCIReadDword(bus, slot, func, barOffset);

    if(barLow == 0)
    {
        return frg::null_opt;
    }

    bool isMMIO = !(barLow & 1);
    bool isPrefetch = isMMIO && (barLow & 0b1000);
    bool is64Bit = isMMIO && ((barLow & 0b110) == 0b100);

    if(is64Bit)
    {
        barHigh = PCIReadDword(bus, slot, func, barOffset + 4);
    }

    base = ((barHigh << 32) | barLow) & ~(isMMIO ? 0xFull : 0x3ull);

    PCIWriteDword(bus, slot, func, barOffset, 0xFFFFFFFF);

    barSizeLow = PCIReadDword(bus, slot, func, barOffset);

    PCIWriteDword(bus, slot, func, barOffset, barLow);

    if(is64Bit)
    {
        PCIWriteDword(bus, slot, func, barOffset + 4, 0xFFFFFFFF);

        barSizeHigh = PCIReadDword(bus, slot, func, barOffset + 4);

        PCIWriteDword(bus, slot, func, barOffset + 4, barHigh);
    }
    else
    {
        barSizeHigh = 0xFFFFFFFF;
    }

    size = ~(((barSizeHigh << 32) | barSizeLow) & ~(isMMIO ? 0xFull : 0x3ull));

    if(isMMIO == false)
    {
        base &= 0xFFFF;
        size &= 0xFFFF;
    }

    return PCIBar { isMMIO ? PCIBarType::MMIO : PCIBarType::IO, base, size, isPrefetch };
}

void PCIDevice::AttachTo(PCIBus *bus)
{
    parent = bus;

    bus->AttachDevice(this);
}

static inline int PCIBridgePin(int pin, int dev)
{
    return (((pin - 1) + (dev % 4)) % 4) + 1;
}

void PCIDevice::RouteIRQ(lai_state_t *state) {

    uint8_t irqPin = this->irqPin;

    if(irqPin == 0)
    {
        return;
    }

    auto bus = parent;
    auto busDevice = this;
    lai_variable_t *prt = NULL;

    for(;;)
    {
        if(bus == nullptr)
        {
            break;
        }

        bus->FetchPRT(state);

        if(bus->HasPRT() == false)
        {
            irqPin = PCIBridgePin(irqPin, bus->Device()->Slot());
        }
        else
        {
            prt = bus->PRT();

            break;
        }

        if(bus->Parent() != nullptr)
        {
            busDevice = bus->Device();
        }

        bus = bus->Parent();
    }

    if(prt == nullptr)
    {
        return;
    }

    lai_prt_iterator iter = LAI_PRT_ITERATOR_INITIALIZER(prt);
    lai_api_error err;

	while ((err = lai_pci_parse_prt(&iter)) == 0)
    {
		if (iter.slot == busDevice->Slot() && (iter.function == busDevice->Func() || iter.function == -1) &&
            iter.pin == (irqPin - 1))
        {
			// TODO: care about flags for the IRQ

            //TODO: Add ioapic
			//irq = ioapic_get_vector_by_gsi(iter.gsi);
			gsi = iter.gsi;
			break;
		}
	}
}

uint8_t PCIDevice::Bus() const
{
    return bus;
}

uint8_t PCIDevice::Slot() const
{
    return slot;
}

uint8_t PCIDevice::Func() const
{
    return func;
}

uint16_t PCIDevice::Vendor() const
{
    return vendor;
}

uint16_t PCIDevice::Device() const
{
    return device;
}

uint8_t PCIDevice::BaseClass() const
{
    return baseClass;
}

uint8_t PCIDevice::SubClass() const
{
    return subClass;
}

uint8_t PCIDevice::ProgIf() const
{
    return progIf;
}

uint8_t PCIDevice::HeaderType() const
{
    return headerType;
}

bool PCIDevice::MultiFunction() const
{
    return multiFunction;
}

uint8_t PCIDevice::SecBus() const
{
    return secBus;
}

uint8_t PCIDevice::IRQPin() const
{
    return irqPin;
}

uint8_t PCIDevice::IRQ() const
{
    return irq;
}

uint8_t PCIDevice::GSI() const
{
    return gsi;
}

PCIBus *PCIDevice::Parent()
{
    return parent;
}
