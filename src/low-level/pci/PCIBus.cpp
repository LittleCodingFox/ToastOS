#include "PCI.hpp"
#include "ports/Ports.hpp"
#include <frg/unique.hpp>

box<vector<frg::unique_ptr<PCIDevice, frg_allocator>>> devices;
box<vector<frg::unique_ptr<PCIBus, frg_allocator>>> PCIBuses;

PCIBus::PCIBus(uint8_t bus, PCIDevice *device) : parent(nullptr), device(device), children { frg_allocator::get() }, bus(bus) {}

void PCIBus::SetACPINode(lai_nsnode_t *node)
{
    acpiNode = node;
}

void PCIBus::FindNode(lai_state_t *state)
{
    if(acpiNode != nullptr)
    {
        return;
    }

    if(parent)
    {
        parent->FindNode(state);
    }

    if(parent && parent->acpiNode != nullptr)
    {
        return;
    }

    if(parent == nullptr || device == nullptr)
    {
        return;
    }

    acpiNode = lai_pci_find_device(parent->acpiNode, device->Slot(), device->Func(), state);
}

lai_variable_t *PCIBus::PRT()
{
    return &acpiprt;
}

void PCIBus::FetchPRT(lai_state_t *state)  {

    if(triedPRTFetch)
    {
        return;
    }

    triedPRTFetch = true;

    if(acpiNode == nullptr)
    {
        return;
    }

    lai_nsnode_t *prtHandle = lai_resolve_path(acpiNode, "_PRT");

    if(prtHandle != nullptr)
    {
        hasPRT = lai_eval(&acpiprt, prtHandle, state) == false;
    }
}

bool PCIBus::HasPRT() const
{
    return hasPRT;
}

void PCIBus::Enumerate(lai_state_t *state)
{
    for(uint8_t slot = 0; slot < 32; slot++)
    {
        for(uint8_t func = 0; func < 8; func++)
        {
            auto device = frg::make_unique<PCIDevice>(frg_allocator::get(), bus, slot, func);

            if(device->Exists() == false)
            {
                continue;
            }

            device->Init();
            device->AttachTo(this);

            if(device->BaseClass() == 0x06 && device->SubClass() == 0x04)
            {
                auto bridge = frg::make_unique<PCIBus>(frg_allocator::get(), device->SecBus(), device.get());

                bridge->AttachTo(this);
                bridge->FindNode(state);
                bridge->Enumerate(state);

                PCIBuses->push_back(std::move(bridge));
            }

            devices->push_back(std::move(device));
        }
    }
}

void PCIBus::AttachDevice(PCIDevice *device)
{
    children.push_back(std::move(device));
}

void PCIBus::AttachTo(PCIBus *bus)
{
    parent = bus;
}

PCIBus *PCIBus::Parent()
{
    return parent;
}

PCIDevice *PCIBus::Device()
{
    return device;
}

static void PCIFindRootBuses(lai_state_t *state)
{
    LAI_CLEANUP_VAR lai_variable_t PCIPNPID = LAI_VAR_INITIALIZER;
    LAI_CLEANUP_VAR lai_variable_t PCIEPNPID = LAI_VAR_INITIALIZER;
    lai_eisaid(&PCIPNPID, "PNP0A03");
    lai_eisaid(&PCIEPNPID, "PNP0A08");

    lai_nsnode_t *sbHandle = lai_resolve_path(NULL, "\\_SB_");

    if(sbHandle == nullptr)
    {
        return;
    }

    lai_ns_child_iterator iter = LAI_NS_CHILD_ITERATOR_INITIALIZER(sbHandle);
    lai_nsnode *node;

    while((node = lai_ns_child_iterate(&iter)))
    {
        if(lai_check_device_pnp_id(node, &PCIPNPID, state) && lai_check_device_pnp_id(node, &PCIEPNPID, state))
        {
            continue;
        }

        LAI_CLEANUP_VAR lai_variable_t busNumber = LAI_VAR_INITIALIZER;
        uint64_t bbnResult = 0;
        lai_nsnode_t *bbnHandle = lai_resolve_path(node, "_BBN");

        if(bbnHandle != nullptr)
        {
            if(lai_eval(&busNumber, bbnHandle, state))
            {
                continue;
            }

            lai_obj_get_integer(&busNumber, &bbnResult);
        }

        auto bus = frg::make_unique<PCIBus>(frg_allocator::get(), static_cast<uint8_t>(bbnResult));

        bus->SetACPINode(node);

        PCIBuses->push_back(std::move(bus));
    }
}

void InitializePCI()
{
    devices.initialize(frg_allocator::get());
    PCIBuses.initialize(frg_allocator::get());

    LAI_CLEANUP_STATE lai_state_t state;
    lai_init_state(&state);

    PCIFindRootBuses(&state);

    size_t initialCount = PCIBuses->size();

    for(size_t i = 0; i < initialCount; i++)
    {
        (*PCIBuses.get())[i]->Enumerate(&state);
    }

    for(auto &device : *devices.get())
    {
        device->RouteIRQ(&state);

        if(device->IRQ())
        {
            DEBUG_OUT("[pci] device %02x.%02x.%01x routed to gsi %u (irq %u)", device->Bus(), device->Slot(), device->Func(), device->GSI(), device->IRQ());
        }
    }

    for(auto &device : *devices.get())
    {
        DEBUG_OUT("[pci] %02x.%02x.%01x %04x:%04x class %02x subclass %02x progIf %02x",
            device->Bus(), device->Slot(), device->Func(), device->Vendor(),
            device->Device(), device->BaseClass(), device->SubClass(), device->ProgIf());

        for(int i = 0; i < 6; i++)
        {
            auto bar = device->FetchBar(i);

            if(!bar)
            {
                continue;
            }

            DEBUG_OUT("[pci]\tbar %d - %c%c %0161x %0161x", i, bar->type == PCIBarType::IO ? 'i' : 'm',
                bar->prefetch ? 'p' : ' ', bar->address, bar->length);
        }
    }
}
