#include "PIT.hpp"
#include "ports/Ports.hpp"
#include "debug.hpp"
#include "interrupts/IDT.hpp"
#include "lapic/LAPIC.hpp"
#include "ioapic/IOAPIC.hpp"

#define TIMER_FREQUENCY 100

uint16_t PITGetCurrentCount()
{
    outport8(0x43, 0x00);
    uint8_t low = inport8(0x40);
    uint8_t high = inport8(0x40) << 8;

    return ((uint16_t)high << 8) | low;
}

void PITSetReloadValue(uint16_t count)
{
    outport8(0x43, 0x34);
    outport8(0x40, (uint8_t)count);
    outport8(0x40, (uint8_t)(count >> 8));
}

void PITSetFrequency(uint64_t frequency)
{
    uint8_t divisor = PIT_DIVIDEND / frequency;

    if(PIT_DIVIDEND % frequency > frequency / 2)
    {
        divisor++;
    }

    PITSetReloadValue((uint16_t)divisor);
}

static void PITTimerHandler(InterruptStack *stack)
{
    LAPICEOI();
    //TODO
}

void InitializePIT()
{
    PITSetFrequency(TIMER_FREQUENCY);

    uint8_t timerVector = idt.AllocateVector();

    interrupts.RegisterHandler(timerVector, PITTimerHandler);

    IOAPICSetIRQRedirect(LAPICGetID(), timerVector, 0, true);
}
