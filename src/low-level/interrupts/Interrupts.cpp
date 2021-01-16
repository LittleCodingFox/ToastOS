#include "Interrupts.hpp"
#include "Panic.hpp"
#include "ports/Ports.hpp"
#include "framebuffer/FramebufferRenderer.hpp"
#include "printf/printf.h"
#include "keyboard/keyboard.hpp"

__attribute__((interrupt)) void PageFault_Handler(struct interrupt_frame* frame)
{
    Panic("Page Fault Detected");
    while(true);
}

__attribute__((interrupt)) void DoubleFault_Handler(struct interrupt_frame* frame)
{
    Panic("Double Fault Detected");
    while(true);
}

__attribute__((interrupt)) void GPFault_Handler(struct interrupt_frame* frame)
{
    Panic("General Protection Fault Detected");
    while(true);
}

__attribute__((interrupt)) void KeyboardInt_Handler(struct interrupt_frame* frame)
{
    uint8_t scancode = inport8(0x60);

    HandleKeyboardKeyPress(scancode);

    PIC_EndMaster();
}

void PIC_EndMaster()
{
    outport8(PIC1_COMMAND, PIC_EOI);
}

void PIC_EndSlave()
{
    outport8(PIC2_COMMAND, PIC_EOI);
    outport8(PIC1_COMMAND, PIC_EOI);
}
   

void RemapPIC()
{
    uint8_t a1, a2; 

    a1 = inport8(PIC1_DATA);
    io_wait();
    a2 = inport8(PIC2_DATA);
    io_wait();

    outport8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outport8(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outport8(PIC1_DATA, 0x20);
    io_wait();
    outport8(PIC2_DATA, 0x28);
    io_wait();

    outport8(PIC1_DATA, 4);
    io_wait();
    outport8(PIC2_DATA, 2);
    io_wait();

    outport8(PIC1_DATA, ICW4_8086);
    io_wait();
    outport8(PIC2_DATA, ICW4_8086);
    io_wait();

    outport8(PIC1_DATA, a1);
    io_wait();
    outport8(PIC2_DATA, a2);
}