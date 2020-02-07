;*
;* x86_64-bios/bootboot.inc
;*
;* Copyright (C) 2017 - 2020 bzt (bztsrc@gitlab)
;*
;* Permission is hereby granted, free of charge, to any person
;* obtaining a copy of this software and associated documentation
;* files (the "Software"), to deal in the Software without
;* restriction, including without limitation the rights to use, copy,
;* modify, merge, publish, distribute, sublicense, and/or sell copies
;* of the Software, and to permit persons to whom the Software is
;* furnished to do so, subject to the following conditions:
;*
;* The above copyright notice and this permission notice shall be
;* included in all copies or substantial portions of the Software.
;*
;* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
;* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
;* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
;* DEALINGS IN THE SOFTWARE.
;*
;* This file is part of the BOOTBOOT Protocol package.
;* @brief The BOOTBOOT structure
;*
; ------ !!! WARNING: MUST MATCH ../bootboot.h !!! ------

bootboot = 8000h





; this define is in the 18th line of bootboot.h
bootboot_MAGIC     equ   'BOOT'

; minimum protocol level:
;  hardcoded kernel name, static kernel memory addresses
PROTOCOL_MINIMAL   equ   0
; static protocol level:
;  kernel name parsed from environment, static kernel memory addresses
PROTOCOL_STATIC    equ   1
; dynamic protocol level:
;  kernel name parsed from environment, kernel memory addresses parsed from ELF symbols
PROTOCOL_DYNAMIC   equ   2
; big-endian flag
PROTOCOL_BIGENDIAN equ      080h

; loader types, just informational
LOADER_BIOS   equ    0
LOADER_UEFI   equ    4
LOADER_RPI    equ    8

; framebuffer pixel format, only 32 bits supported
FB_ARGB       equ 0
FB_RGBA       equ 1
FB_ABGR       equ 2
FB_BGRA       equ 3



;  mmap entry, type is stored in least significant tetrad of size
virtual at 0
    mmap_ent.ptr:         dq	0
    mmap_ent.size:        dq	0
end virtual
; we don't have entry field macros for asm
; realsize = size & 0xFFFFFFFFFFF0
; type = size & 0xF


MMAP_USED         equ 0
MMAP_FREE         equ 1
MMAP_ACPI         equ 2
MMAP_MMIO         equ 3

INITRD_MAXSIZE     equ 16 ; Mb

virtual at bootboot
   ; first 64 bytes is platform independent
    bootboot.magic:       dd	0
    bootboot.size:        dd	0
    bootboot.protocol:    db	1
    bootboot.fb_type:     db	0
    bootboot.numcores:    dw	0
    bootboot.bspid:       dw	0
    bootboot.timezone:    dw	0
    bootboot.datetime:    dq	0
    bootboot.initrd_ptr:  dq	0
    bootboot.initrd_size: dq	0
    bootboot.fb_ptr:      dq	0
    bootboot.fb_size:     dd	0
    bootboot.fb_width:    dd	0
    bootboot.fb_height:   dd	0
    bootboot.fb_scanline: dd	0

   ; the rest (64 bytes) is platform specific

   ; x86_64
      bootboot.acpi_ptr:    dq	0
      bootboot.smbi_ptr:    dq	0
      bootboot.efi_ptr:     dq	0
      bootboot.mp_ptr:      dq	0
      bootboot.unused:      dq	0,0,0,0

     bootboot.mmap:
end virtual







