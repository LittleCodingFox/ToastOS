#include "LAPIC.hpp"
#include "interrupts/IDT.hpp"
#include "debug.hpp"
#include "paging/PageTableManager.hpp"
#include "smp/SMP.hpp"
#include "registers/Registers.hpp"
#include "pit/PIT.hpp"
#include "Panic.hpp"

#define LAPIC_REG_ID            0x20 // LAPIC ID
#define LAPIC_REG_EOI           0x0b0 // End of interrupt
#define LAPIC_REG_SPURIOUS      0x0f0
#define LAPIC_REG_CMCI          0x2f0 // LVT Corrected machine check interrupt
#define LAPIC_REG_ICR0          0x300 // Interrupt command register
#define LAPIC_REG_ICR1          0x310
#define LAPIC_REG_LVT_TIMER     0x320
#define LAPIC_REG_TIMER_INITCNT 0x380 // Initial count register
#define LAPIC_REG_TIMER_CURCNT  0x390 // Current count register
#define LAPIC_REG_TIMER_DIV     0x3e0
#define LAPIC_EOI_ACK           0x00

static uint8_t timerVector = 0;

static inline uint32_t LAPICRead(uint32_t reg)
{
    return *((volatile uint32_t *)((uintptr_t)0xFEE00000 + HIGHER_HALF_MEMORY_OFFSET + reg));
}

static inline void LAPICWrite(uint32_t reg, uint32_t value)
{
    *((volatile uint32_t *)((uintptr_t)0xFEE00000 + HIGHER_HALF_MEMORY_OFFSET + reg)) = value;
}

static inline void LAPICTimerStop()
{
    LAPICWrite(LAPIC_REG_TIMER_INITCNT, 0);
    LAPICWrite(LAPIC_REG_LVT_TIMER, 1 << 16);
}

static void LAPICTimerHandler(InterruptStack *stack)
{
    LAPICEOI();

    CPUInfo *info = CurrentCPUInfo();

    if(info != nullptr && info->timerFunction != NULL)
    {
        info->timerFunction(stack);
    }
}

void InitializeLAPIC()
{
    if((Registers::ReadMSR(0x1b) & 0xfffff000) != 0xfee00000)
    {
        Panic("Invalid LAPIC MSR");
    }

    LAPICWrite(LAPIC_REG_SPURIOUS, LAPICRead(LAPIC_REG_SPURIOUS) | (1 << 8) | 0xFF);

    if(timerVector == 0)
    {
        timerVector = idt.AllocateVector();
        idt.SetIST(timerVector, 1);
    }

    interrupts.RegisterHandler(timerVector, LAPICTimerHandler);

    LAPICWrite(LAPIC_REG_LVT_TIMER, LAPICRead(LAPIC_REG_LVT_TIMER) | (1 << 8) | timerVector);

    LAPICTimerCalibrate();
}

void LAPICEOI()
{
    LAPICWrite(LAPIC_REG_EOI, LAPIC_EOI_ACK);
}

void LAPICTimerOneShot(uint32_t us, void *function)
{
    LAPICTimerStop();

    bool interruptsEnabled = interrupts.InterruptsEnabled();

    interrupts.DisableInterrupts();

    CPUInfo *info = CurrentCPUInfo();

    if(info != nullptr)
    {
        info->timerFunction = (void (*)(InterruptStack *))function;
    }

    if(interruptsEnabled)
    {
        interrupts.EnableInterrupts();
    }

    uint32_t ticks = us * (info->LAPICFrequency / 1000000);

    LAPICWrite(LAPIC_REG_LVT_TIMER, timerVector);
    LAPICWrite(LAPIC_REG_TIMER_DIV, 0);
    LAPICWrite(LAPIC_REG_TIMER_INITCNT, ticks);
}

void LAPISendIPI(uint32_t LAPICID, uint32_t vector)
{
    LAPICWrite(LAPIC_REG_ICR1, LAPICID << 24);
    LAPICWrite(LAPIC_REG_ICR0, vector);
}

void LAPICTimerCalibrate()
{
    LAPICTimerStop();

    LAPICWrite(LAPIC_REG_LVT_TIMER, (1 << 16) | 0xFF);
    LAPICWrite(LAPIC_REG_TIMER_DIV, 0);
    PITSetReloadValue(0xFFFF);

    int initTick = PITGetCurrentCount();
    int samples = 0xFFFFF;

    LAPICWrite(LAPIC_REG_TIMER_INITCNT, (uint32_t)samples);

    while(LAPICRead(LAPIC_REG_TIMER_CURCNT) != 0);

    int finalTick = PITGetCurrentCount();
    int totalTicks = initTick - finalTick;
    CPUInfo *info = CurrentCPUInfo();

    if(info != nullptr)
    {
        info->LAPICFrequency = (samples / totalTicks) * PIT_DIVIDEND;

        DEBUG_OUT("LAPIC frequency for %i: %u", info->APICID, info->LAPICFrequency);
    }

    LAPICTimerStop();
}

uint32_t LAPICGetID()
{
    return LAPICRead(LAPIC_REG_ID);
}
