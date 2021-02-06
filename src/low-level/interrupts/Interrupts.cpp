#include <string.h>
#include "Interrupts.hpp"
#include "IDT.hpp"
#include "Panic.hpp"
#include "ports/Ports.hpp"
#include "framebuffer/FramebufferRenderer.hpp"
#include "printf/printf.h"
#include "registers/Registers.hpp"
#include "keyboard/Keyboard.hpp"

Interrupts interrupts;

void breakpointHandler(InterruptStack *stack);
void pageFaultHandler(InterruptStack *stack);
void keyboardHandler(InterruptStack *stack);

static const char* exception_messages[] = { "Division By Zero",
                                            "Debug",
                                            "Non Maskable Interrupt",
                                            "Breakpoint",
                                            "Into Detected Overflow",
                                            "Out of Bounds",
                                            "Invalid Opcode",
                                            "No Coprocessor",

                                            "Double Fault",
                                            "Coprocessor Segment Overrun",
                                            "Bad TSS",
                                            "Segment Not Present",
                                            "Stack Fault",
                                            "General Protection Fault",
                                            "Page Fault",
                                            "Unknown Interrupt",

                                            "Coprocessor Fault",
                                            "Alignment Check",
                                            "Machine Check",
                                            "Reserved",
                                            "Reserved",
                                            "Reserved",
                                            "Reserved",
                                            "Reserved",

                                            "Reserved",
                                            "Reserved",
                                            "Reserved",
                                            "Reserved",
                                            "Reserved",
                                            "Reserved",
                                            "Reserved",
                                            "Reserved" };

void Interrupts::init()
{
    memset(handlers, 0, sizeof(handlers));

    idt.init();

    // start initialization
    outport8(PIC1, 0x11);
    outport8(PIC2, 0x11);

    // set IRQ base numbers for each PIC
    outport8(PIC1_DATA, IRQ_BASE);
    outport8(PIC2_DATA, IRQ_BASE + 8);

    // use IRQ number 2 to relay IRQs from the slave PIC
    outport8(PIC1_DATA, 0x04);
    outport8(PIC2_DATA, 0x02);

    // finish initialization
    outport8(PIC1_DATA, 0x01);
    outport8(PIC2_DATA, 0x01);

    // mask all IRQs
    outport8(PIC1_DATA, 0x00);
    outport8(PIC2_DATA, 0x00);

    // Exceptions
    idt.registerInterrupt(0, (uint64_t)exc0);
    idt.registerInterrupt(1, (uint64_t)exc1);
    idt.registerInterrupt(2, (uint64_t)exc2);
    idt.registerInterrupt(3, (uint64_t)exc3);
    idt.registerInterrupt(4, (uint64_t)exc4);
    idt.registerInterrupt(5, (uint64_t)exc5);
    idt.registerInterrupt(6, (uint64_t)exc6);
    idt.registerInterrupt(7, (uint64_t)exc7);
    idt.registerInterrupt(8, (uint64_t)exc8);
    idt.registerInterrupt(9, (uint64_t)exc9);
    idt.registerInterrupt(10, (uint64_t)exc10);
    idt.registerInterrupt(11, (uint64_t)exc11);
    idt.registerInterrupt(12, (uint64_t)exc12);
    idt.registerInterrupt(13, (uint64_t)exc13);
    idt.registerInterrupt(14, (uint64_t)exc14);
    idt.registerInterrupt(15, (uint64_t)exc15);
    idt.registerInterrupt(16, (uint64_t)exc16);
    idt.registerInterrupt(17, (uint64_t)exc17);
    idt.registerInterrupt(18, (uint64_t)exc18);
    idt.registerInterrupt(19, (uint64_t)exc19);
    idt.registerInterrupt(20, (uint64_t)exc20);
    idt.registerInterrupt(21, (uint64_t)exc21);
    idt.registerInterrupt(22, (uint64_t)exc22);
    idt.registerInterrupt(23, (uint64_t)exc23);
    idt.registerInterrupt(24, (uint64_t)exc24);
    idt.registerInterrupt(25, (uint64_t)exc25);
    idt.registerInterrupt(26, (uint64_t)exc26);
    idt.registerInterrupt(27, (uint64_t)exc27);
    idt.registerInterrupt(28, (uint64_t)exc28);
    idt.registerInterrupt(29, (uint64_t)exc29);
    idt.registerInterrupt(30, (uint64_t)exc30);
    idt.registerInterrupt(31, (uint64_t)exc31);

    // Hardware interrupts
    idt.registerInterrupt(IRQ0, (uint64_t)irq0);
    idt.registerInterrupt(IRQ1, (uint64_t)irq1);
    idt.registerInterrupt(IRQ2, (uint64_t)irq2);
    idt.registerInterrupt(IRQ3, (uint64_t)irq3);
    idt.registerInterrupt(IRQ4, (uint64_t)irq4);
    idt.registerInterrupt(IRQ5, (uint64_t)irq5);
    idt.registerInterrupt(IRQ6, (uint64_t)irq6);
    idt.registerInterrupt(IRQ7, (uint64_t)irq7);
    idt.registerInterrupt(IRQ8, (uint64_t)irq8);
    idt.registerInterrupt(IRQ9, (uint64_t)irq9);
    idt.registerInterrupt(IRQ10, (uint64_t)irq10);
    idt.registerInterrupt(IRQ11, (uint64_t)irq11);
    idt.registerInterrupt(IRQ12, (uint64_t)irq12);

    // Specific handlers for exceptions.
    registerHandler(EXCEPTION_BP, breakpointHandler);
    registerHandler(EXCEPTION_PF, pageFaultHandler);
    registerHandler(IRQ1, keyboardHandler);

    idt.load();

    enableInterrupts();
}

void Interrupts::enableInterrupts()
{
    __asm__("sti");
}

void Interrupts::disableInterrupts()
{
    __asm__("cli");
}

void interruptIntHandler(InterruptStack stack)
{
    InterruptHandler handler = interrupts.getHandler(stack.id);

    if (handler != NULL)
    {
        handler(&stack);

        return;
    }

    Panic("received interrupt (see below)\n\n"
        "  %d - %s\n\n"
        "  error_code          = %#x\n"
        "  instruction_pointer = %p\n"
        "  code_segment        = %#x\n"
        "  cpu_flags           = %#x\n"
        "  stack_pointer       = %p\n"
        "  stack_segment       = %#x\n"
        "\n"
        "  rax = 0x%08x    rbx = 0x%08x    rcx = 0x%08x\n"
        "  rdx = 0x%08x    rsi = 0x%08x    rdi = 0x%08x\n"
        "  rbp = 0x%08x    r8  = 0x%08x    r9  = 0x%08x\n"
        "  r10 = 0x%08x    r11 = 0x%08x    r12 = 0x%08x\n"
        "  r13 = 0x%08x    r14 = 0x%08x    r15 = 0x%08x",
        stack.id,
        exception_messages[stack.id],
        stack.error_code,
        stack.instruction_pointer,
        stack.code_segment,
        stack.cpu_flags,
        stack.stack_pointer,
        stack.stack_segment,
        stack.rax,
        stack.rbx,
        stack.rcx,
        stack.rdx,
        stack.rsi,
        stack.rdi,
        stack.rbp,
        stack.r8,
        stack.r9,
        stack.r10,
        stack.r11,
        stack.r12,
        stack.r13,
        stack.r14,
        stack.r15);
}

void interruptIRQHandler(InterruptStack stack)
{
    InterruptHandler handler = interrupts.getHandler(stack.id);

    if (handler != NULL)
    {
        handler(&stack);
    }

    if (stack.id >= 40)
    {
        outport8(PIC2, PIC_EOI);
    }

    outport8(PIC1, PIC_EOI);
}

void Interrupts::registerHandler(uint64_t id, InterruptHandler handler)
{
    handlers[id] = handler;
}

void keyboardHandler(InterruptStack *stack)
{
    uint8_t scancode = inport8(0x60);

    HandleKeyboardKeyPress(scancode);

    outport8(PIC1, PIC_EOI);
}

void breakpointHandler(InterruptStack *stack)
{
    printf("Exception: BREAKPOINT\n"
            "  instruction_pointer = %p\n"
            "  code_segment        = %x\n"
            "  cpu_flags           = %#x\n"
            "  stack_pointer       = %p\n"
            "  stack_segment       = %x\n",
            stack->instruction_pointer,
            stack->code_segment,
            stack->cpu_flags,
            stack->stack_pointer,
            stack->stack_segment);
}

void pageFaultHandler(InterruptStack* stack)
{
    uint64_t error_code = stack->error_code;
    uint8_t is_present = (error_code >> 0) & 1;
    uint8_t is_write = (error_code >> 1) & 1;
    uint8_t is_user = (error_code >> 2) & 1;
    uint8_t is_reserved_write = (error_code >> 3) & 1;
    uint8_t is_instruction_fetch = (error_code >> 4) & 1;

    Panic("Exception: PAGE FAULT\n"
            "  accessed address    = %p\n"
            "  error_code          = %#x\n"
            "  error details:\n"
            "    present           = %c\n"
            "    write             = %c\n"
            "    user              = %c\n"
            "    reserved write    = %c\n"
            "    instruction fetch = %c\n"
            "  instruction_pointer = %p\n"
            "  code_segment        = %#x\n"
            "  cpu_flags           = %#x\n"
            "  stack_pointer       = %p\n"
            "  stack_segment       = %#x\n"
            "\n"
            "  rax = 0x%08x    rbx = 0x%08x    rcx = 0x%08x\n"
            "  rdx = 0x%08x    rsi = 0x%08x    rdi = 0x%08x\n"
            "  rbp = 0x%08x    r8  = 0x%08x    r9  = 0x%08x\n"
            "  r10 = 0x%08x    r11 = 0x%08x    r12 = 0x%08x\n"
            "  r13 = 0x%08x    r14 = 0x%08x    r15 = 0x%08x",
            Registers::readCR2(),
            error_code,
            is_present != 0 ? 'Y' : 'N',
            is_write != 0 ? 'Y' : 'N',
            is_user != 0 ? 'Y' : 'N',
            is_reserved_write != 0 ? 'Y' : 'N',
            is_instruction_fetch != 0 ? 'Y' : 'N',
            stack->instruction_pointer,
            stack->code_segment,
            stack->cpu_flags,
            stack->stack_pointer,
            stack->stack_segment,
            stack->rax,
            stack->rbx,
            stack->rcx,
            stack->rdx,
            stack->rsi,
            stack->rdi,
            stack->rbp,
            stack->r8,
            stack->r9,
            stack->r10,
            stack->r11,
            stack->r12,
            stack->r13,
            stack->r14,
            stack->r15);
}
