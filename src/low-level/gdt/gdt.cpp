#include "gdt.hpp"

__attribute__((aligned(0x1000)))
GDT DefaultGDT = {
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = 0x00,
        .limitFlags = 0x00,
        .baseHigh = 0
    }, // null
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessKernelCode,
        .limitFlags = 0xA0, 
        .baseHigh = 0
    }, // kernel code segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessKernelData,
        .limitFlags = 0x80,
        .baseHigh = 0
    }, // kernel data segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessUserData,
        .limitFlags = 0x80,
        .baseHigh = 0
    }, // user data segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessUserCode,
        .limitFlags = 0xA0,
        .baseHigh = 0
    }, // user code segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessTSS,
        .limitFlags = 0xA0,
        .baseHigh = 0,
    }, //TSSLow
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = 0,
        .limitFlags = 0,
        .baseHigh = 0,
    } //TSSHigh
};