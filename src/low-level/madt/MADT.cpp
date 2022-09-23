#include "MADT.hpp"
#include "acpi/ACPI.hpp"
#include "Panic.hpp"

box<vector<MADTLAPIC *>> MADTLAPICs;
box<vector<MADTIOAPIC *>> MADTIOAPICs;
box<vector<MADTISO *>> MADTISOs;
box<vector<MADTNMI *>> MADTNMIs;

void InitializeMADT()
{
    MADTLAPICs.initialize();
    MADTIOAPICs.initialize();
    MADTISOs.initialize();
    MADTNMIs.initialize();

    if(madt == nullptr)
    {
        Panic("No MADT in system");
    }

    size_t offset = 0;

    for(;;)
    {
        if(madt->header.length - sizeof(MADT) - offset < 2)
        {
            break;
        }

        MADTHeader *header = (MADTHeader *)(madt->entries + offset);

        switch(header->id)
        {
            case 0:
                DEBUG_OUT("[madt] Found local APIC #%lu", MADTLAPICs->size());
                MADTLAPICs->push_back((MADTLAPIC *)header);

                break;
                
            case 1:
                DEBUG_OUT("[madt] Found IO APIC #%lu", MADTIOAPICs->size());
                MADTIOAPICs->push_back((MADTIOAPIC *)header);

                break;

            case 2:
                DEBUG_OUT("[madt] Found ISO #%lu", MADTISOs->size());
                MADTISOs->push_back((MADTISO *)header);

                break;

            case 4:
                DEBUG_OUT("[madt] Found NMI #%lu", MADTNMIs->size());
                MADTNMIs->push_back((MADTNMI *)header);

                break;
        }

        offset += header->length < 2 ? 2 : header->length;
    }
}
