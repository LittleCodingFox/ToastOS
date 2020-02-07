;*
;* x86_64-bios/boot.asm
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
;* @brief Stage1 loader, compatible with BIOS boot specification and
;* El Torito CD-ROM boot in "no emulation" mode
;*

;*********************************************************************
;*                             Macros                                *
;*********************************************************************

;LBA packet fields
virtual at lba_packet
lbapacket.size: dw      ?
lbapacket.count:dw      ?
lbapacket.addr0:dw      ?
lbapacket.addr1:dw      ?
lbapacket.sect0:dw      ?
lbapacket.sect1:dw      ?
lbapacket.sect2:dw      ?
lbapacket.sect3:dw      ?
end virtual

;memory locations
ldr.header      equ     800h        ;position of 2nd stage loader
ldr.executor    equ     804h        ;ptr to init code

;Writes a message on screen.
macro print msg
{
if ~ msg eq si
            push        si
            mov         si, msg
end if
            call        printfunc
if ~ msg eq si
            pop         si
end if
}

;*********************************************************************
;*                             code                                  *
;*********************************************************************

;-----------------ENTRY POINT called by BIOS--------------------
            ORG         0600h
            USE16

bootboot_record:
            jmp         short .skipid
            nop
.system:    db          "BOOTBOOT",0
            ;skip BPB area so that we can use this
            ;boot code on a FAT volume if needed
            db          05Ah-($-$$) dup 0
.skipid:    ;relocate our code to offset 0h:600h
            cli
            xor         ax, ax
            mov         ss, ax
            mov         sp, 600h
            push        ax
            pop         es
            push        cs
            pop         ds
            ;find our position in memory.
            call        .getaddr
.getaddr:   pop         si
            sub         si, .getaddr-bootboot_record
            cld
            mov         di, sp
            ;clear data area 500h-600h
            sub         di, 100h
            mov         cx, 80h
            repnz       stosw
            ;and copy ourselves to 600h
            mov         cx, 100h
            repnz       movsw
            ;have to clear ds, because cs is set to 7c0 when booted from El Torito
            push        es
            pop         ds
            jmp         0:.start

.start:     ;save boot drive code
            mov         byte [drive], dl
            ;initialize lba packet
            mov         byte [lbapacket.size], 16
            mov         byte [lbapacket.addr0+1], 08h   ;to address 800h
            ;check for lba presistance - floppy not supported any more
            ;we use USB sticks as removable media for a long time
            cmp         dl, byte 80h
            jl          .nolba
.notfloppy: mov         ah, byte 41h
            mov         bx, word 55AAh
            clc
            int         13h
            jc          .nolba
            cmp         bx, word 0AA55h
            jne         .nolba
            test        cl, byte 1
            jnz         .lbaok
.nolba:     mov         si, lbanotf
            jmp         diefunc
.lbaok:     ;try to load stage2 - it's a continous area on disk
            ;started at given sector with maximum size of 7000h bytes
            mov         si, stage2_addr
            mov         di, lbapacket.sect0
            push        bx
            push        di
            ;set up for hard-drive
            movsw
            movsw
            mov         byte [lbapacket.count], 56
            ;query cdrom status to see if it's a cdrom
            mov         ax, word 4B01h
            mov         dl, byte [drive]
            mov         si, spc_packet
            mov         byte [si], 13h
            mov         byte [si+2], 0h ;clear drive number
            clc
            int         13h
            pop         di
            jc          @f
            ;some buggy BIOSes (like bochs') fail to set carry flag and ax properly
            cmp         byte [spc_packet+2], 0h
            jz          @f
            ;lba=lba/4
            clc
            rcr         word [di+2], 1
            rcr         word [di], 1
            clc
            rcr         word [di+2], 1
            rcr         word [di], 1
            mov         byte [lbapacket.count], 14
            ;load sectors
@@:         mov         ah, byte 42h
            mov         dl, byte [drive]
            mov         si, lba_packet
            int         13h
            pop         bx

            ;do we have a 2nd stage loader?
.chk:       cmp         word [ldr.header], bx
            jne         .nostage2
            cmp         byte [ldr.header+3], 0E9h
            jne         .nostage2
            ;invoke stage2 real mode code
            mov         ax, [ldr.executor]
            add         ax, ldr.executor+3
            jmp         ax

.nostage2:  ;try to load stage2 from a RAID mirror
            inc         byte [drive]
            cmp         byte [drive], 8Fh
            jle         .lbaok
.nostage2err:
            mov         si, stage2notf
            ;fall into the diefunc code

;*********************************************************************
;*                          functions                                *
;*********************************************************************
;writes the reason, waits for a key and reboots.
diefunc:
            print       bootboot_record.system
            print       panic
            call        printfunc
            mov         si, found
            call        printfunc
            sti
            xor         ax, ax
            int         16h
            mov         al, 0FEh
            out         64h, al
            jmp         far 0FFFFh:0    ;invoke BIOS POST routine

;ds:si zero terminated string to write
printfunc:
            lodsb
            or          al, al
            jz          .end
            mov         ah, byte 0Eh
            mov         bx, word 11
            int         10h
            jmp         printfunc
.end:       ret

;*********************************************************************
;*                          data area                                *
;*********************************************************************

panic:      db          "-PANIC: ",0
lbanotf:    db          "LBA support",0
stage2notf: db          "FS0:\BOOTBOOT\LOADER",0
found:      db          " not found",10,13,0
drive:      db          0
lba_packet: db          16 dup 0
spc_packet: db          13h dup 0
            db          01B0h-($-$$) dup 0

;right before the partition table some data
stage2_addr:dd          0FFFFFFFFh,0    ;1B0h 2nd stage loader address
                                        ;this should be set by mkfs

diskid:     dd          0               ;1B8h WinNT expects it here
            dw          0

;1BEh first partition entry

            ;padding and magic
            db          01FEh-($-$$) dup 0
            db          55h,0AAh
bootboot_record_end:
