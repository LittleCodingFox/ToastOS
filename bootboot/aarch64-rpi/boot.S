/*
 * aarch64-rpi/boot.S
 *
 * Copyright (C) 2017 - 2020 bzt (bztsrc@gitlab)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief Boot loader for the Raspberry Pi 3+ ARMv8
 *
 */
.section ".text.boot"

.global _start

/*********************************************************************
 *                Entry point called by start.elf                    *
 *********************************************************************/
_start:
    // magic
    b       1f
    .ascii  "BOOTBOOT"
    // set stack before our code
1:  ldr     x1, =_start
    // set up EL1
    mrs     x0, CurrentEL
    and     x0, x0, #12
    // running at EL3?
    cmp     x0, #12
    bne     1f
    msr     sp_el2, x1
    mov     x2, #0x5b1
    msr     scr_el3, x2
    mov     x2, #0x3c9
    msr     spsr_el3, x2
    adr     x2, 1f
    msr     elr_el3, x2
    eret
    // running at EL2?
1:  cmp     x0, #4
    beq     1f
    msr     sp_el1, x1
    // set up exception handlers
    ldr     x2, =_vectors
    msr     vbar_el2, x2
    // enable CNTP for EL1
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0
    msr     cnthp_ctl_el2, xzr
    // initialize virtual MPIDR
    mrs     x0, midr_el1
    mrs     x2, mpidr_el1
    msr     vpidr_el2, x0
    msr     vmpidr_el2, x2
    // disable coprocessor traps
    mov     x0, #0x33FF
    msr     cptr_el2, x0
    msr     hstr_el2, xzr
    mov     x0, #(3 << 20)
    msr     cpacr_el1, x0
    // enable AArch64 in EL1
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    mrs     x0, hcr_el2
    // Setup SCTLR access
    mov     x2, #0x0800
    movk    x2, #0x30d0, lsl #16
    msr     sctlr_el1, x2
    // change exception level to EL1
    mov     x2, #0x3c4
    msr     spsr_el2, x2
    adr     x2, 1f
    msr     elr_el2, x2
    eret
    // set up exception handlers
1:  ldr     x2, =_vectors
    msr     vbar_el1, x2
    // disable execution level dependent stacks, so that we have only 4 vectors
    msr     SPSel, #0
    // read cpu id, start slave cores
    mrs     x7, mpidr_el1
    and     x7, x7, #3
    cbnz    x7, 3f
    // failsafe: if startup.elf does not start all cores (depends on config.txt), and qemu expects 0xD8
    mov     x2, #0x40000000
    mov     x3, #0xd8
    str     w1, [x2, #0x9C]     // write _start address to core1's mailbox 3
    str     w1, [x3, #8]
    str     w1, [x2, #0xAC]     // write _start address to core2's mailbox 3
    str     w1, [x3, #16]
    str     w1, [x2, #0xBC]     // write _start address to core3's mailbox 3
    str     w1, [x3, #24]
    sev
    // clear bss
    ldr     x2, =__bss_start
    ldr     w3, =__bss_size
1:  cbz     w3, 2f
    str     xzr, [x2], #8
    sub     w3, w3, #1
    cbnz    w3, 1b
2:  mov     sp, x1
    // jump to C code
    bl      bootboot_main
3:  b       bootboot_startcore

    .align 11
_vectors:
    .align  7
    mov     x0, #0
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    mrs     x5, sctlr_el1
    mrs     x6, tcr_el1
    b       uart_exc
    .align  7
    mov     x0, #1
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    mrs     x5, sctlr_el1
    mrs     x6, tcr_el1
    b       uart_exc
    .align  7
    mov     x0, #2
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    mrs     x5, sctlr_el1
    mrs     x6, tcr_el1
    b       uart_exc
    .align  7
    mov     x0, #3
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    mrs     x5, sctlr_el1
    mrs     x6, tcr_el1
    b       uart_exc
