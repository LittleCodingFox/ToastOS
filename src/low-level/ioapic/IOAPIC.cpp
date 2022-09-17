#include "IOAPIC.hpp"
#include "paging/PageTableManager.hpp"
#include "madt/MADT.hpp"
#include "Panic.hpp"

static uint32_t IOAPICRead(MADTIOAPIC *IOAPIC, uint32_t reg)
{
    uint64_t base = (uint64_t)IOAPIC->address + HIGHER_HALF_MEMORY_OFFSET;
    *(volatile uint32_t *)base = reg;

    return *(volatile uint32_t *)(base + 16);
}

static void IOAPICWrite(MADTIOAPIC *IOAPIC, uint32_t reg, uint32_t value)
{
    uint64_t base = (uint64_t)IOAPIC->address + HIGHER_HALF_MEMORY_OFFSET;
    *(volatile uint32_t *)base = reg;
    *(volatile uint32_t *)(base + 16) = value;
}

static size_t IOAPICGSICount(MADTIOAPIC *IOAPIC)
{
    return (IOAPICRead(IOAPIC, 1) & 0xFF0000) >> 16;
}

static MADTIOAPIC *IOAPICFromGSI(uint32_t gsi)
{
    for(size_t i = 0; i < MADTIOAPICs->size(); i++)
    {
        MADTIOAPIC *IOAPIC = (*MADTIOAPICs.get())[i];

        if(gsi >= IOAPIC->gsib && gsi < IOAPIC->gsib + IOAPICGSICount(IOAPIC))
        {
            return IOAPIC;
        }
    }

    Panic("Failed to get IOAPIC from GSI %u", gsi);

    return nullptr;
}

void IOAPICSetIRQRedirect(uint32_t LAPICID, uint8_t vector, uint8_t irq, bool status)
{
    for(size_t i = 0; i < MADTISOs->size(); i++)
    {
        MADTISO *iso = (*MADTISOs.get())[i];

        if(iso->IRQSource != irq)
        {
            continue;
        }

        IOAPICSetGSIRedirect(LAPICID, vector, iso->gsi, iso->flags, status);

        return;
    }

    IOAPICSetGSIRedirect(LAPICID, vector, irq, 0, status);
}

void IOAPICSetGSIRedirect(uint32_t LAPICID, uint8_t vector, uint8_t gsi, uint16_t flags, bool status)
{
    MADTIOAPIC *IOAPIC = IOAPICFromGSI(gsi);

    uint64_t redirect = vector;

    if ((flags & (1 << 1)) != 0)
    {
        redirect |= (1 << 13);
    }

    if ((flags & (1 << 3)) != 0)
    {
        redirect |= (1 << 15);
    }

    if (!status)
    {
        redirect |= (1 << 16);
    }

    redirect |= (uint64_t)LAPICID << 56;

    uint32_t IORedirectTable = (gsi - IOAPIC->gsib) * 2 + 16;

    IOAPICWrite(IOAPIC, IORedirectTable, (uint32_t)redirect);

    IOAPICWrite(IOAPIC, IORedirectTable + 1, (uint32_t)(redirect >> 32));
}
