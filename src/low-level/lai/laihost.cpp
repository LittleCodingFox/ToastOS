#include <lai/host.h>
#include <string.h>
#include "debug.hpp"
#include "Panic.hpp"
#include "acpi/ACPI.hpp"
#include "pci/PCI.hpp"
#include "paging/PageTableManager.hpp"
#include "ports/Ports.hpp"

extern "C" {

    void *laihost_malloc(size_t length)
    {
        void *mem = malloc(length);

        memset(mem, 0, length);

        return mem;
    }

    void *laihost_realloc(void *ptr, size_t length, size_t old)
    {
        void *mem = realloc(ptr, length);

        memset(mem, 0, length);

        return mem;
    }

    void laihost_free(void *ptr, size_t length)
    {
        free(ptr);
    }

    void laihost_log(int level, const char *message)
    {
        (void)level;

        DEBUG_OUT("lai: %s", message);
    }

    __attribute__((noreturn)) void laihost_panic(const char *message)
    {
        Panic("lai: %s", message);
        __builtin_unreachable();
    }

    void *laihost_scan(const char *signature, size_t index)
    {
        DEBUG_OUT("lai: scan for %.*s", 4, signature);
        return ACPI::FindTable(xsdt, signature);
    }

    void *laihost_map(size_t address, size_t count)
    {
        (void)count;

        if((address + count) > 0xFFFFFFFF)
        {
            Panic("lai: laihost_map called with address, count pair which goes out of bounds");
        }

        return (void *)TranslateToHighHalfMemoryAddress(address);
    }

    void laihost_outb(uint16_t port, uint8_t val)
    {
        outport8(port, val);
    }

    void laihost_outw(uint16_t port, uint16_t val)
    {
        outport16(port, val);
    }

    void laihost_outd(uint16_t port, uint32_t val)
    {
        outport32(port, val);
    }

    uint8_t laihost_inb(uint16_t port)
    {
        return inport8(port);
    }

    uint16_t laihost_inw(uint16_t port)
    {
        return inport16(port);
    }

    uint32_t laihost_ind(uint16_t port)
    {
        return inport32(port);
    }

    void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off, uint8_t val)
    {
        if(seg)
        {
            Panic("TODO");
        }

        PCIWriteByte(bus, dev, fun, off, val);
    }

    uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off)
    {
        if(seg)
        {
            Panic("TODO");
        }

        return PCIReadByte(bus, dev, fun, off);
    }

    void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off, uint16_t val)
    {
        if(seg)
        {
            Panic("TODO");
        }

        PCIWriteWord(bus, dev, fun, off, val);
    }

    uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off)
    {
        if(seg)
        {
            Panic("TODO");
        }

        return PCIReadWord(bus, dev, fun, off);
    }

    void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off, uint32_t val)
    {
        if(seg)
        {
            Panic("TODO");
        }

        PCIWriteDword(bus, dev, fun, off, val);
    }

    uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t fun, uint16_t off)
    {
        if(seg)
        {
            Panic("TODO");
        }

        return PCIReadDword(bus, dev, fun, off);
    }

    void laihost_sleep(uint64_t time) {}
}
