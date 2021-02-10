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
        .limitFlags = 0xa0, 
        .baseHigh = 0
    }, // kernel code segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessKernelData,
        .limitFlags = 0xa0,
        .baseHigh = 0
    }, // kernel data segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = 0x00,
        .limitFlags = 0x00,
        .baseHigh = 0
    }, // user null
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessUserCode,
        .limitFlags = 0xa0,
        .baseHigh = 0
    }, // user code segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessUserData,
        .limitFlags = 0xa0,
        .baseHigh = 0
    }, // user data segment
};