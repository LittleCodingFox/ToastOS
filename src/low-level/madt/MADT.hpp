#pragma once

#include <stdint.h>
#include <stddef.h>
#include "kernel.h"

struct PACKED MADTHeader
{
    uint8_t id;
    uint8_t length;
};

struct PACKED MADTLAPIC
{
    MADTHeader header;
    uint8_t processorID;
    uint8_t APICID;
    uint32_t flags;
};

struct PACKED MADTIOAPIC
{
    MADTHeader header;
    uint8_t APICID;
    uint8_t reserved;
    uint32_t address;
    uint32_t gsib;
};

struct PACKED MADTISO
{
    MADTHeader header;
    uint8_t BUSSource;
    uint8_t IRQSource;
    uint32_t gsi;
    uint16_t flags;
};

struct PACKED MADTNMI
{
    MADTHeader header;
    uint8_t processor;
    uint16_t flags;
    uint8_t lint;
};

extern box<vector<MADTLAPIC *>> MADTLAPICs;
extern box<vector<MADTIOAPIC *>> MADTIOAPICs;
extern box<vector<MADTISO *>> MADTISOs;
extern box<vector<MADTNMI *>> MADTNMIs;

void InitializeMADT();
