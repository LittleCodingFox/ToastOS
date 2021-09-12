#include "../gdt/gdt.hpp"
#include "tss.hpp"

TSS tss = { 0 };

void InitializeTSS()
{
    uint64_t tssBase = ((uint64_t)&tss);

    DefaultGDT.tssLow.baseLow = tssBase & 0xFFFF;
    DefaultGDT.tssLow.baseMiddle = (tssBase >> 16) & 0xFF;
    DefaultGDT.tssLow.baseHigh = (tssBase >> 24) & 0xFF;
    DefaultGDT.tssLow.limitLow = sizeof(tss);
    DefaultGDT.tssHigh.limitLow = (tssBase >> 32) & 0xFFFF;
    DefaultGDT.tssHigh.baseLow = (tssBase >> 48) & 0xFFFF;

    __asm__("ltr %%ax" : : "a"(GDTTSSSegment));
}
