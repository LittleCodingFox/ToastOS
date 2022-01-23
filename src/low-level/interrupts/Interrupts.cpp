#include <string.h>
#include "Interrupts.hpp"
#include "IDT.hpp"
#include "Panic.hpp"
#include "ports/Ports.hpp"
#include "framebuffer/FramebufferRenderer.hpp"
#include "printf/printf.h"
#include "registers/Registers.hpp"
#include "keyboard/Keyboard.hpp"
#include "process/Process.hpp"
#include "debug.hpp"
#include "serial/Serial.hpp"

Interrupts interrupts;

void BreakpointHandler(InterruptStack *stack);
void PageFaultHandler(InterruptStack *stack);
void DoubleFaultHandler(InterruptStack *stack);
void KeyboardHandler(InterruptStack *stack);
void SwitchProcess(InterruptStack *stack);
void SyscallHandler(InterruptStack *stack);

static const char *exception_messages[] =
{
    "Division By Zero",
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
    "Reserved"
};

void Interrupts::Init()
{
    memset(handlers, 0, sizeof(handlers));

    idt.Init();

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

    // unmask all IRQs
    outport8(PIC1_DATA, 0x00);
    outport8(PIC2_DATA, 0x00);

    // Exceptions
    idt.RegisterInterrupt(0, (uint64_t)exc0);
    idt.RegisterInterrupt(1, (uint64_t)exc1);
    idt.RegisterInterrupt(2, (uint64_t)exc2);
    idt.RegisterInterrupt(3, (uint64_t)exc3);
    idt.RegisterInterrupt(4, (uint64_t)exc4);
    idt.RegisterInterrupt(5, (uint64_t)exc5);
    idt.RegisterInterrupt(6, (uint64_t)exc6);
    idt.RegisterInterrupt(7, (uint64_t)exc7);
    idt.RegisterInterrupt(8, (uint64_t)exc8);
    idt.RegisterInterrupt(9, (uint64_t)exc9);
    idt.RegisterInterrupt(10, (uint64_t)exc10);
    idt.RegisterInterrupt(11, (uint64_t)exc11);
    idt.RegisterInterrupt(12, (uint64_t)exc12);
    idt.RegisterInterrupt(13, (uint64_t)exc13);
    idt.RegisterInterrupt(14, (uint64_t)exc14);
    idt.RegisterInterrupt(15, (uint64_t)exc15);
    idt.RegisterInterrupt(16, (uint64_t)exc16);
    idt.RegisterInterrupt(17, (uint64_t)exc17);
    idt.RegisterInterrupt(18, (uint64_t)exc18);
    idt.RegisterInterrupt(19, (uint64_t)exc19);
    idt.RegisterInterrupt(20, (uint64_t)exc20);
    idt.RegisterInterrupt(21, (uint64_t)exc21);
    idt.RegisterInterrupt(22, (uint64_t)exc22);
    idt.RegisterInterrupt(23, (uint64_t)exc23);
    idt.RegisterInterrupt(24, (uint64_t)exc24);
    idt.RegisterInterrupt(25, (uint64_t)exc25);
    idt.RegisterInterrupt(26, (uint64_t)exc26);
    idt.RegisterInterrupt(27, (uint64_t)exc27);
    idt.RegisterInterrupt(28, (uint64_t)exc28);
    idt.RegisterInterrupt(29, (uint64_t)exc29);
    idt.RegisterInterrupt(30, (uint64_t)exc30);
    idt.RegisterInterrupt(31, (uint64_t)exc31);

    // Custom interrupts
    idt.RegisterInterrupt(0x30, (uint64_t)exc48, 0, 1);
    idt.RegisterInterrupt(0x80, (uint64_t)exc128, 3, 2);

    // Hardware interrupts
    idt.RegisterInterrupt(IRQ0, (uint64_t)irq0);
    idt.RegisterInterrupt(IRQ1, (uint64_t)irq1);
    idt.RegisterInterrupt(IRQ2, (uint64_t)irq2);
    idt.RegisterInterrupt(IRQ3, (uint64_t)irq3);
    idt.RegisterInterrupt(IRQ4, (uint64_t)irq4);
    idt.RegisterInterrupt(IRQ5, (uint64_t)irq5);
    idt.RegisterInterrupt(IRQ6, (uint64_t)irq6);
    idt.RegisterInterrupt(IRQ7, (uint64_t)irq7);
    idt.RegisterInterrupt(IRQ8, (uint64_t)irq8);
    idt.RegisterInterrupt(IRQ9, (uint64_t)irq9);
    idt.RegisterInterrupt(IRQ10, (uint64_t)irq10);
    idt.RegisterInterrupt(IRQ11, (uint64_t)irq11);
    idt.RegisterInterrupt(IRQ12, (uint64_t)irq12);

    // Specific handlers for exceptions.
    RegisterHandler(EXCEPTION_BP, BreakpointHandler);
    RegisterHandler(EXCEPTION_PF, PageFaultHandler);
    RegisterHandler(EXCEPTION_DF, DoubleFaultHandler);
    RegisterHandler(IRQ1, KeyboardHandler);
    RegisterHandler(0x30, SwitchProcess);
    RegisterHandler(0x80, SyscallHandler);

    idt.Load();

    EnableInterrupts();
}

void Interrupts::EnableInterrupts()
{
    __asm__("sti");
}

void Interrupts::DisableInterrupts()
{
    __asm__("cli");
}

void interruptIntHandler(InterruptStack stack)
{
    InterruptHandler handler = interrupts.GetHandler(stack.id);

    if (handler != NULL)
    {
        handler(&stack);

        return;
    }
    else
    {
        Interrupts::HandlerInfo *argHandler = interrupts.GetHandlerArg(stack.id);

        if(argHandler != NULL && argHandler->handler != NULL)
        {
            argHandler->handler(&stack, argHandler->data);
        }
    }

    Panic("received interrupt (see below)\n\n"
        "  %lld - %s\n\n"
        "  error_code          = %#llx\n"
        "  instruction_pointer = %p\n"
        "  code_segment        = %#llx\n"
        "  cpu_flags           = %#llx\n"
        "  stack_pointer       = %p\n"
        "  stack_segment       = %#llx\n"
        "\n"
        "  rax = 0x%016llx    rbx = 0x%016llx    rcx = 0x%016llx\n"
        "  rdx = 0x%016llx    rsi = 0x%016llx    rdi = 0x%016llx\n"
        "  rbp = 0x%016llx    r8  = 0x%016llx    r9  = 0x%016llx\n"
        "  r10 = 0x%016llx    r11 = 0x%016llx    r12 = 0x%016llx\n"
        "  r13 = 0x%016llx    r14 = 0x%016llx    r15 = 0x%016llx\n"
        "  cr3 = 0x%016llx\n",
        stack.id,
        exception_messages[stack.id],
        stack.errorCode,
        stack.instructionPointer,
        stack.codeSegment,
        stack.cpuFlags,
        stack.stackPointer,
        stack.stackSegment,
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
        stack.r15,
        Registers::ReadCR3());
}

void interruptIRQHandler(InterruptStack stack)
{
    InterruptHandler handler = interrupts.GetHandler(stack.id);

    if (handler != NULL)
    {
        handler(&stack);
    }
    else
    {
        Interrupts::HandlerInfo *argHandler = interrupts.GetHandlerArg(stack.id);

        if(argHandler != NULL && argHandler->handler != NULL)
        {
            argHandler->handler(&stack, argHandler->data);
        }
    }

    if (stack.id >= 40)
    {
        outport8(PIC2, PIC_EOI);
    }

    outport8(PIC1, PIC_EOI);
}

void Interrupts::RegisterHandler(uint64_t id, InterruptHandler handler)
{
    handlers[id] = handler;
}

void Interrupts::RegisterHandler(uint64_t id, InterruptHandlerArg handler, void *data)
{
    HandlerInfo info;
    info.handler = handler;
    info.data = data;

    argHandlers[id] = info;
}

void KeyboardHandler(InterruptStack *stack)
{
    uint8_t scancode = inport8(0x60);

    HandleKeyboardKeyPress(scancode);

    outport8(PIC1, PIC_EOI);
}

void BreakpointHandler(InterruptStack *stack)
{
    Panic("Exception: BREAKPOINT\n"
            "  instruction_pointer = %p\n"
            "  code_segment        = %x\n"
            "  cpu_flags           = %#x\n"
            "  stack_pointer       = %p\n"
            "  stack_segment       = %x\n",
            stack->instructionPointer,
            stack->codeSegment,
            stack->cpuFlags,
            stack->stackPointer,
            stack->stackSegment);
}

void DoubleFaultHandler(InterruptStack *stack)
{
    Panic("Exception: DOUBLE FAULT\n"
            "  code_segment        = %x\n"
            "  cpu_flags           = %#x\n"
            "  stack_pointer       = %p\n"
            "  stack_segment       = %x\n",
            stack->codeSegment,
            stack->cpuFlags,
            stack->stackPointer,
            stack->stackSegment);
}

void PageFaultHandler(InterruptStack* stack)
{
    uint64_t error_code = stack->errorCode;
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
            "  rax = 0x%016llx    rbx = 0x%016llx    rcx = 0x%016llx\n"
            "  rdx = 0x%016llx    rsi = 0x%016llx    rdi = 0x%016llx\n"
            "  rbp = 0x%016llx    r8  = 0x%016llx    r9  = 0x%016llx\n"
            "  r10 = 0x%016llx    r11 = 0x%016llx    r12 = 0x%016llx\n"
            "  r13 = 0x%016llx    r14 = 0x%016llx    r15 = 0x%016llx\n"
            "  cr3 = 0x%016llx\n",
            Registers::ReadCR2(),
            error_code,
            is_present != 0 ? 'Y' : 'N',
            is_write != 0 ? 'Y' : 'N',
            is_user != 0 ? 'Y' : 'N',
            is_reserved_write != 0 ? 'Y' : 'N',
            is_instruction_fetch != 0 ? 'Y' : 'N',
            stack->instructionPointer,
            stack->codeSegment,
            stack->cpuFlags,
            stack->stackPointer,
            stack->stackSegment,
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
            stack->r15,
            Registers::ReadCR3());
}
