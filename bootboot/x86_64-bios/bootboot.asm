;*
;* x86_64-bios/bootboot.asm
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
;* @brief Booting code for BIOS, MultiBoot, El Torito, Linux boot
;*
;*  Stage2 loader, compatible with GRUB and BIOS boot specification
;*  1.0.1 (even expansion ROM), El Torito "no emulation" CDROM boot,
;*  as well as Linux boot protocol.
;*
;*  text segment occupied: 800-7C00, bss: 8000-x
;*
;*  Memory map
;*      0h -  600h reserved for the system
;*    600h -  800h stage1 (MBR/VBR, boot.bin)
;*    800h - 6C00h stage2 (this)
;*   6C00h - 7C00h stack
;*   8000h - 9000h bootboot structure
;*   9000h - A000h environment
;*   A000h - B000h disk buffer / PML4
;*   B000h - C000h PDPE, higher half core 4K slots
;*   C000h - D000h PDE 4K
;*   D000h - E000h PTE 4K
;*   E000h - F000h PDPE, 4G physical RAM identity mapped 2M
;*   F000h -10000h PDE 2M
;*  10000h -11000h PDE 2M
;*  11000h -12000h PDE 2M
;*  12000h -13000h PDE 2M
;*  13000h -14000h PTE 4K
;*  14000h -A0000h core stacks (1k per core)
;*
;*  At first big enough free hole, initrd. Usually at 1Mbyte.
;*

DEBUG equ 1

;get Core boot parameter block
include "bootboot.inc"

;VBE filter (available, has additional info, color, graphic, linear fb)
VBE_MODEFLAGS   equ         1+2+8+16+128

;*********************************************************************
;*                             Macros                                *
;*********************************************************************

;Writes a message on screen.
macro real_print msg
{
if ~ msg eq si
            push        si
            mov         si, msg
end if
            call        real_printfunc
if ~ msg eq si
            pop         si
end if
}

;protected and real mode are functions, because we have to switch beetween
macro       real_protmode
{
            USE16
            call            near real_protmodefunc
            USE32
}

macro       prot_realmode
{
            USE32
            call            near prot_realmodefunc
            USE16
}

;edx:eax sector, edi:pointer
macro       prot_readsector
{
            call            near prot_readsectorfunc
}

macro       DBG msg
{
if DEBUG eq 1
            real_print      msg
end if
}

macro       DBG32 msg
{
if DEBUG eq 1
            prot_realmode
            real_print      msg
            real_protmode
end if
}

virtual at 0
    bpb.jmp             db 3 dup 0
    bpb.oem             db 8 dup 0
    bpb.bps             dw 0
    bpb.spc             db 0
    bpb.rsc             dw 0
    bpb.nf              db 0 ;16
    bpb.nr              dw 0
    bpb.ts16            dw 0
    bpb.media           db 0
    bpb.spf16           dw 0 ;22
    bpb.spt             dw 0
    bpb.nh              dw 0
    bpb.hs              dd 0
    bpb.ts32            dd 0
    bpb.spf32           dd 0 ;36
    bpb.flg             dd 0
    bpb.rc              dd 0 ;44
    bpb.vol             db 6 dup 0
    bpb.fst             db 8 dup 0 ;54
    bpb.dmy             db 20 dup 0
    bpb.fst2            db 8 dup 0 ;84
end virtual

virtual at 0
    fatdir.name         db 8 dup 0
    fatdir.ext          db 3 dup 0
    fatdir.attr         db 9 dup 0
    fatdir.ch           dw 0
    fatdir.attr2        dd 0
    fatdir.cl           dw 0
    fatdir.size         dd 0
end virtual

;*********************************************************************
;*                             header                                *
;*********************************************************************
;offs   len desc
;  0     2  expansion ROM magic (AA55h)
;  2     1  size in blocks (40h)
;  3     1  magic E9h
;  4     2  real mode entry point (relative)
;  6     2  checksum
;  8     8  magic 'BOOTBOOT'
; 16    10  zeros, at least one and a padding
; 26     2  pnp ptr, must be zero
; 28     4  flags, must be zero
; 32    32  MultiBoot header with protected mode entry point
; 64     x  free to use
;497   127  Linux x86 boot protocol header
;any format can follow.

            USE16
            ORG         800h
;BOOTBOOT stage2 header (64 bytes)
loader:     db          55h,0AAh                ;ROM magic
            db          (loader_end-loader)/512 ;size in 512 blocks
.executor:  jmp         near realmode_start     ;entry point
.checksum:  dw          0                       ;checksum
.name:      db          "BOOTBOOT"
            dw          0
            dd          0, 0
.pnpptr:    dw          0
.flags:     dd          0
MB_MAGIC    equ         01BADB002h
MB_FLAGS    equ         010001h
            align           8
.mb_header: dd          MB_MAGIC        ;magic
            dd          MB_FLAGS        ;flags
            dd          -(MB_MAGIC+MB_FLAGS)    ;checksum (0-magic-flags)
            dd          .mb_header      ;our location (GRUB should load us here)
            dd          0800h           ;the same... load start
            dd          07C00h          ;load end
            dd          0h              ;no bss
            dd          multiboot_start ;entry point

;no segments or sections, code comes right after the header

;*********************************************************************
;*                             code                                  *
;*********************************************************************

;----------------Multiboot stub-----------------
            USE32
multiboot_start:
            cli
            cld
            ;clear drive code, initrd ptr and environment
            xor         edx, edx
            mov         edi, 9000h
            mov         dword [edi], edx
            mov         dword [bootboot.initrd_ptr], edx
            mov         dword [bootboot.initrd_size], edx
            ;no GRUB environment available?
            cmp         eax, 2BADB002h
            jne         @f
            ;save drive code for boot device
            mov         dl, byte [ebx+12]
            ;is there a module? mod_count!=0
            cmp         dword [ebx+20], 0
            jz          @f
            ;mod_addr!=0
            mov         eax, dword [ebx+24]
            or          eax, eax
            jz          @f
            ;mods[0].end
            mov         ecx, dword [eax+4]
            sub         ecx, dword [eax]
            mov         dword [bootboot.initrd_size], ecx
            ;mods[0].start
            mov         ecx, dword [eax]
            mov         dword [bootboot.initrd_ptr], ecx
            inc         byte [hasinitrd]
            ;mod_count>1?
            cmp         dword [ebx+20], 1
            jbe         @f
            ;mods[1], copy environment (4k)
            mov         esi, dword [eax+16]
            mov         ecx, 1024
            repnz       movsd
            inc         byte [hasconfig]
@@:         lgdt        [GDT_value]
            mov         ax, DATA_BOOT   ;clear shadow segment registers
            mov         ds, ax
            mov         es, ax
            mov         ss, ax
            xor         esp, esp        ;GRUB leaves the upper 16 bits non-zero, we must clear it
            jmp         CODE_BOOT:.real ;load 16 bit mode segment into cs
            USE16
.real:      mov         eax, CR0
            and         eax, 07FFFFFFEh ;switching back to real mode
            mov         CR0, eax
            xor         ax, ax
            mov         ds, ax          ;load segment registers DS and CS
            jmp         0:@f
@@:         lidt        [idt16]         ;restore IDT as newer GRUBs mess it up
            ;fallthrough realmode_start

;-----------realmode-protmode stub-------------
realmode_start:
            cli
            cld
            mov         sp, 7C00h
            xor         ax, ax
            mov         es, ax
            mov         ss, ax
            ;relocate ourself from ROM to RAM if necessary
            call        .getaddr
.getaddr:   pop         si
            mov         ax, cs
            or          ax, ax
            jnz         .reloc
            cmp         si, .getaddr
            je          .noreloc
.reloc:     mov         ds, ax
            mov         di, loader
            sub         si, .getaddr-loader
            mov         cx, (loader_end-loader)/2
            repnz       movsw
            xor         ax, ax
            mov         ds, ax
            jmp         0:.clrdl
.noreloc:   or          dl, dl
            jnz         @f
.clrdl:     mov         dl, 80h
@@:         mov         byte [bootdev], dl

            ;-----initialize serial port COM1,115200,8N1------
            mov         ax, 0401h
            xor         bx, bx
            mov         cx, 030Bh
            xor         dx, dx
            int         14h
            real_print  starting

            in          al, 060h    ; read key
            in          al, 061h    ; ack
            out         061h, al

            DBG         dbg_cpu

            ;-----check CPU-----
            ;at least 286?
            pushf
            pushf
            pop         dx
            xor         dh,40h
            push        dx
            popf
            pushf
            pop         bx
            popf
            cmp         dx, bx
            jne         .cpuerror
            mov         ebp, 200000h
            ;check for 386
            ;look for cpuid instruction
            pushfd
            pop         eax
            mov         ebx, eax
            xor         eax, ebp
            and         ebx, ebp
            push        eax
            popfd
            pushfd
            pop         eax
            and         eax, ebp
            xor         eax, ebx
            shr         eax, 21
            and         al, 1
            jz          .cpuerror
            ;ok, now we can get cpu feature flags
            mov         eax, 1
            cpuid
            shr         al, 4
            shr         ebx, 24
            mov         word [bootboot.bspid], bx
            ;look for minimum family
            cmp         ax, 0600h
            jb          .cpuerror
            ;look for minimum feature flags
            ;so we have PAE?
            bt          edx, 6
            jnc         .cpuerror
            ;what about MSR?
            bt          edx, 5
            jnc         .cpuerror
            ;and can we use long mode (LME)?
            mov         eax, 80000000h
            mov         ebp, eax
            inc         ebp
            cpuid
            cmp         eax, ebp
            jb          .cpuerror
            mov         eax, ebp
            cpuid
            ;long mode
            bt          edx, 29
            jc          .cpuok
.cpuerror:  mov         si, noarch
            jmp         real_diefunc
.cpuok:     ;okay, we can do 64 bit!

            DBG         dbg_A20

            ;-----enable A20-----
            ;no problem even if a20 is already turned on.
            stc
            mov         ax, 2401h   ;BIOS enable A20 function
            int         15h
            jnc         a20ok
            ;keyboard nightmare
            call        a20wait
            mov         al, 0ADh
            out         64h, al
            call        a20wait
            mov         al, 0D0h
            out         64h, al
            call        a20wait2
            in          al, 60h
            push            ax
            call        a20wait
            mov         al, 0D1h
            out         64h, al
            call        a20wait
            pop         ax
            or          al, 2
            out         60h, al
            call        a20wait
            mov         al, 0AEh
            out         64h, al
            jmp         a20ok

;--- Linux x86 boot protocol header ---
            db          01F1h-($-$$) dup 090h
hdr:
setup_sects:        db          (loader_end-loader-511)/512
root_flags:         dw          0
syssize:            dd          (loader_end-loader)/16
ram_size:           dw          0
vid_mode:           dw          0
root_dev:           dw          0
boot_flag:          dw          0AA55h
                    db          0EBh            ; short jmp
                    db          start_of_setup-@f   ; no space to jump to realmode_start directly
@@:                 db          "HdrS"
                    dw          020eh
realmode_swtch:     dd          0
start_sys_seg:      dw          0
                    dw          0               ; we don't have Linux kernel version...
type_of_loader:     db          0FFh
loadflags:          db          0
setup_move_size:    dw          08000h
code32_start:       dd          0               ; we dont' use this either...
ramdisk_image:      dd          0
ramdisk_size:       dd          0
bootsect_kludge:    dd          0
heap_end_ptr:       dd          loader_end+1024-512 ; we don't care, we set our stack directly
ext_loader_ver:     db          0
ext_loader_type:    db          0
cmd_line_ptr:       dd          0
initrd_addr_max:    dd          07fffffffh
kernel_alignment:   dd          16
relocatable_kernel: db          0
min_alignment:      db          4
xloadflags:         dw          0
cmdline_size:       dd          0
hardware_subarch:   dd          0
hardware_subarch_data: dq       0
payload_offset:     dd          0
payload_length:     dd          0
setup_data:         dq          0
pref_address:       dq          90000h          ; qemu does not handle anything else
init_size:          dd          loader_end-loader
handover_offset:    dd          0
acpi_rsdp_addr:     dq          0
start_of_setup:
            ; fix: qemu forces address and set CS to 0x9020, but we must jump to segment 0x9000.
            jmp         9000h:realmode_start-loader
; --- end of Linux boot protocol header ---

a20wait:    in          al, 64h
            test        al, 2
            jnz         a20wait
            ret
a20wait2:   in          al, 64h
            test        al, 1
            jz          a20wait2
            ret
a20ok:
            ; wait for a key press, if so use backup initrd
            mov         ecx, dword [046Ch]
            add         ecx, 10  ; ~500ms, 18.2/2
            sti
.waitkey:   pause
            in          al, 064h
            and         al, 1
            jz          @f
            mov         eax, ' BAK'
            mov         dword [bkp], eax
            real_print  backup
            jmp         .waitend
@@:         cmp         dword [046Ch], ecx
            jb          .waitkey
.waitend:   cli

            ;-----detect memory map-----
getmemmap:
            DBG         dbg_mem

            xor         eax, eax
            mov         edi, bootboot.acpi_ptr
            mov         ecx, 16
            repnz       stosd
            cmp         byte [hasconfig], 0
            jnz         @f
            mov         dword [9000h], eax
@@:         cmp         byte [hasinitrd], 0
            jnz         @f
            mov         dword [bootboot.initrd_ptr], eax
            mov         dword [bootboot.initrd_size], eax
@@:         mov         dword [bootboot.initrd_ptr+4], eax
            mov         dword [bootboot.initrd_size+4], eax
            mov         eax, bootboot_MAGIC
            mov         dword [bootboot.magic], eax
            mov         dword [bootboot.size], 128
            mov         dword [bootboot.protocol], PROTOCOL_STATIC or LOADER_BIOS
            mov         di, bootboot.fb_ptr
            mov         cx, 800h-28h
            xor         ax, ax
            repnz       stosw
            mov         di, bootboot.mmap
            xor         ebx, ebx
            clc
.nextmap:   cmp         word [bootboot.size], 4096
            jae         .nomoremap
            mov         edx, 'PAMS'
            xor         ecx, ecx
            mov         cl, 20
            xor         eax, eax
            mov         ax, 0E820h
            int         15h
            jc          .nomoremap
            cmp         eax, 'PAMS'
            jne         .nomoremap
            ;is this the first memory hole? If so, mark
            ;ourself as reserved memory
            cmp         dword [di+4], 0
            jnz         .notfirst
            cmp         dword [di], 0
            jnz         .notfirst
            ; "allocate" memory for loader
            mov         eax, 14000h
            add         dword [di], eax
            sub         dword [di+8], eax
            ;convert E820 memory type to BOOTBOOT memory type
.notfirst:  mov         al, byte [di+16]
            cmp         al, 1
            je          .noov
            cmp         al, 4
            je          .isacpi
            cmp         al, 3
            jne         @f
.isacpi:    mov         al, MMAP_ACPI
            jmp         .noov
            ; everything else reserved
@@:         mov         al, MMAP_USED
.noov:      ;copy memory type to size's least significant byte
            mov         byte [di+8], al
            xor         ecx, ecx
            ;is it ACPI area?
            cmp         al, MMAP_ACPI
            jne         .notacpi
            mov         dword [bootboot.acpi_ptr], edi
            jmp         .entryok
            ;is it free slot?
.notacpi:   cmp         al, MMAP_FREE
            jne         .notmax
.freemem:   ;do we have a ramdisk area?
            cmp         dword [bootboot.initrd_ptr], 0
            jnz         .entryok
            ;is it big enough for the compressed and the inflated ramdisk?
            mov         ebp, (INITRD_MAXSIZE+2)*1024*1024
            shr         ebp, 20
            shl         ebp, 20
            ;is this free memory hole big enough? (first fit)
.sizechk:   mov         eax, dword [di+8]               ;load size
            xor         al, al
            mov         edx, dword [di+12]
            and         edx, 000FFFFFFh
            or          edx, edx
            jnz         .bigenough
            cmp         eax, ebp
            jb          .entryok
.bigenough: mov         eax, dword [di]
            ;save ramdisk pointer
            mov         dword [bootboot.initrd_ptr], eax
.entryok:   ;get limit of memory
            mov         eax, dword [di+8]               ;load size
            xor         al, al
            mov         edx, dword [di+12]
            add         eax, dword [di]                 ;add base
            adc         edx, dword [di+4]
            and         edx, 000FFFFFFh
.notmax:    add         dword [bootboot.size], 16
            ;bubble up entry if necessary
            push        si
            push        di
.bubbleup:  mov         si, di
            cmp         di, bootboot.mmap
            jbe         .swapdone
            sub         di, 16
            ;order by base asc
            mov         eax, dword [si+4]
            cmp         eax, dword [di+4]
            jb          .swapmodes
            jne         .swapdone
            mov         eax, dword [si]
            cmp         eax, dword [di]
            jae         .swapdone
.swapmodes: push        di
            mov         cx, 16/2
@@:         mov         dx, word [di]
            lodsw
            stosw
            mov         word [si-2], dx
            dec         cx
            jnz         @b
            pop         di
            jmp         .bubbleup
.swapdone:  pop         di
            pop         si
            add         di, 16
            cmp         di, bootboot.mmap+4096
            jae         .nomoremap
.skip:      or          ebx, ebx
            jnz         .nextmap
.nomoremap: cmp         dword [bootboot.size], 128
            jne         .E820ok
.noE820:    mov         si, memerr
            jmp         real_diefunc

.E820ok:    ;check if we have memory for the ramdisk
            xor         ecx, ecx
            cmp         dword [bootboot.initrd_ptr], ecx
            jnz         .enoughmem
.nomem:     mov         si, noenmem
            jmp         real_diefunc
.enoughmem:

            ;-----detect system structures-----
            DBG         dbg_systab

            ;do we need that scanning shit?
            mov         eax, dword [bootboot.acpi_ptr]
            or          eax, eax
            jz          @f
            shr         eax, 4
            mov         es, ax
            ;no if E820 map was correct
            cmp         dword [es:0], 'XSDT'
            je          .detsmbi
            cmp         dword [es:0], 'RSDT'
            je          .detsmbi
@@:         inc         dx
            ;get starting address min(EBDA,E0000)
            mov         ah,0C1h
            stc
            int         15h
            mov         bx, es
            jnc         @f
            mov         ax, [ebdaptr]
@@:         cmp         ax, 0E000h
            jb          .acpinext
            mov         ax, 0E000h
            ;detect ACPI ptr
.acpinext:  mov         es, ax
            cmp         dword [es:0], 'RSD '
            jne         .acpinotf
            cmp         dword [es:4], 'PTR '
            jne         .acpinotf
            ;ptr found
            ; do we have XSDT?
            cmp         dword [es:28], 0
            jne         .acpi2
            cmp         dword [es:24], 0
            je          .acpi1
.acpi2:     mov         eax, dword [es:24]
            mov         dword [bootboot.acpi_ptr], eax
            mov         edx, dword [es:28]
            mov         dword [bootboot.acpi_ptr+4], edx
            jmp         .chkacpi
            ; no, fallback to RSDT
.acpi1:     mov         eax, dword [es:16]
@@:         mov         dword [bootboot.acpi_ptr], eax
            xor         edx, edx
            jmp         .chkacpi
.acpinotf:  xor         eax, eax
            mov         ax, es
            inc         ax
            cmp         ax, 0A000h
            jne         @f
            add         ax, 03000h
@@:         ;end of 1Mb?
            or          ax, ax
            jnz         .acpinext
            mov         si, noacpi
            jmp         real_diefunc
.chkacpi:   ; check if memory is marked as ACPI in the memory map
            mov         esi, bootboot.mmap
            mov         edi, bootboot.magic
            add         edi, dword [bootboot.size]
.nextmm:    cmp         edx, dword [si+4]
            jne         .skipmm
            mov         ebx, dword [si]
            cmp         eax, ebx
            jl          .skipmm
            add         ebx, dword [si+8]
            and         bl, 0F0h
            cmp         eax, ebx
            jge         .skipmm
            mov         al, byte [si+8]
            and         al, 0F0h
            or          al, MMAP_ACPI
            mov         byte [si+8], al
            jmp         .detsmbi
.skipmm:    add         si, 16
            cmp         si, di
            jl          .nextmm

            ;detect SMBios tables
.detsmbi:   xor         eax, eax
            mov         ax, 0E000h
            xor         dx, dx
.smbnext:   mov         es, ax
            push            ax
            cmp         dword [es:0], '_SM_'
            je          .smbfound
            cmp         dword [es:0], '_MP_'
            jne         .smbnotf
            shl         eax, 4
            mov         dword [bootboot.mp_ptr], eax
            bts         dx, 2
            jmp         .smbnotf
.smbfound:  shl         eax, 4
            mov         dword [bootboot.smbi_ptr], eax
            bts         dx, 1
.smbnotf:   pop         ax
            bt          ax, 0
            mov         bx, ax
            and         bx, 03h
            inc         ax
            ;end of 1Mb?
            or          ax, ax
            jnz         .smbnext
            ;restore ruined es
.detend:    push        ds
            pop         es

            DBG         dbg_time

            ; ------- BIOS date and time -------
            mov         ah, 4
            int         1Ah
            jc          .nobtime
            ;ch century
            ;cl year
            xchg        ch, cl
            mov         word [bootboot.datetime], cx
            ;dh month
            ;dl day
            xchg        dh, dl
            mov         word [bootboot.datetime+2], dx
            mov         ah, 2
            int         1Ah
            jc          .nobtime
            ;ch hour
            ;cl min
            xchg        ch, cl
            mov         word [bootboot.datetime+4], cx
            ;dh sec
            ;dl daylight saving on/off
            xchg        dh, dl
            mov         word [bootboot.datetime+6], dx
.nobtime:

            ;---- enable protmode ----
            cli
            cld
            lgdt        [GDT_value]
            mov         eax, cr0
            or          al, 1
            mov         cr0, eax
            jmp         CODE_PROT:protmode_start

            ;---- enable protmode on APs ----
ap_trampoline:
            ;--this code will be relocated to the SIPI address --
            cli
            cld
            jmp         0:ap_start
            ;--relocation end--
ap_start:   xor         ax, ax
            mov         ds, ax
            lgdt        [GDT_value]
            mov         eax, cr0
            or          al, 1
            mov         cr0, eax
            jmp         CODE_PROT:@f
            USE32
@@:         mov         ax, DATA_PROT
            mov         ds, ax
            mov         es, ax
            mov         fs, ax
            mov         gs, ax
            mov         ss, ax
            ; spinlock until BSP finishes
@@:         pause
            cmp         byte [bsp_done], 0
            jz          @b
            jmp         longmode_init

;writes the reason, waits for a key and reboots.
prot_diefunc:
            prot_realmode
            USE16
real_diefunc:
            push        si
            real_print  loader.name
            real_print  panic
            pop         si
            call        real_printfunc
            call        real_getchar
            mov         al, 0FEh
            out         64h, al
            jmp         far 0FFFFh:0    ;invoke BIOS POST routine

; get a character from keyboard or from serial line
real_getchar:
            pushf
            sti
            push        si
            push        di
.chkser:    mov         ah, byte 03h
            xor         dx, dx
            int         14h
            bt          ax, 8
            jnc         @f
            mov         ah, byte 02h
            xor         dx, dx
            int         14h
@@:         mov         ah, byte 01h
            int         16h
            jnz         .chkser
            xor         ah, ah
            int         16h
.gotch:     pop         di
            pop         si
            popf
            xor         ah, ah
            ret

;ds:si zero terminated string to write
real_printfunc:
            lodsb
            or          al, al
            jz          .end
            push        si
            push        ax
            mov         ah, byte 0Eh
            mov         bx, word 11
            int         10h
            pop         ax
            mov         ah, byte 01h
            xor         dx, dx
            int         14h
            pop         si
            jmp         real_printfunc
.end:       ret

real_protmodefunc:
            cli
            ;get return address
            xor         ebp, ebp
            pop         bp
            mov         dword [hw_stack], esp
            lgdt        [GDT_value]
            mov         eax, cr0        ;enable protected mode
            or          al, 1
            mov         cr0, eax
            jmp         CODE_PROT:.init

            USE32
.init:      mov         ax, DATA_PROT
            mov         ds, ax
            mov         es, ax
            mov         fs, ax
            mov         gs, ax
            mov         ss, ax
            mov         esp, dword [hw_stack]
            jmp         ebp

prot_realmodefunc:
            cli
            ;get return address
            pop         ebp
            ;save stack pointer
            mov         dword [hw_stack], esp
            jmp         CODE_BOOT:.back     ;load 16 bit mode segment into cs
            USE16
.back:      mov         eax, CR0
            and         al, 0FEh        ;switching back to real mode
            mov         CR0, eax
            xor         ax, ax
            mov         ds, ax          ;load registers 2nd turn
            mov         es, ax
            mov         ss, ax
            jmp         0:.back2
.back2:     mov         sp, word [hw_stack]
            sti
            jmp         bp

            USE32
prot_readsectorfunc:
            push        eax
            push        ecx
            push        esi
            push        edi
            ;load 8 sectors (1 page) or more in low memory
            mov         dword [lbapacket.sect0], eax
            prot_realmode
            ;try all drives from bootdev-8F to support RAID mirror
            mov         dl, byte [bootdev]
            mov         byte [readdev], dl
.again:     mov         ax, word [lbapacket.count]
            mov         word [origcount], ax
            ;query cdrom status to see if it's a cdrom
            mov         ax, word 4B01h
            mov         dl, byte [readdev]
            mov         esi, spc_packet
            mov         byte [si], 13h
            mov         byte [si+2], 0h ;clear drive number
            clc
            int         13h
            jc          @f
            ;some buggy BIOSes (like bochs') fail to set carry flag and ax properly
            cmp         byte [spc_packet+2], 0h
            jz          @f
            ;use 2048 byte sectors instead of 512 if it's a cdrom
            mov         al, byte [lbapacket.sect0]
            and         al, 011b
            or          al, al
            jz          .cdok
            ;this should never happen.
            ; - GPT is loaded from PMBR, from LBA 0 (%4==0)
            ; - ESP is at LBA 128 (%4==0)
            ; - root dir is at LBA 172 (%4==0) for FAT16, and it's cluster aligned for FAT32
            ; - cluster size is multiple of 4 sectors
            mov         si, notcdsect
            jmp         real_diefunc
.cdok:      shr         dword [lbapacket.sect0], 2
            add         word [lbapacket.count], 3
            shr         word [lbapacket.count], 2
            mov         byte [iscdrom], 1
@@:         mov         dl, byte [readdev]
            inc         byte [readdev]
            mov         ah, byte 42h
            mov         esi, lbapacket
            clc
            int         13h
            jnc         @f
            cmp         byte [readdev], 08Fh
            jle         .again
@@:         xor         ebx, ebx
            mov         bl, ah
            real_protmode
            pop         edi
            or          edi, edi
            jz          @f
            push        edi
            ;and copy to addr where it wanted to be (maybe in high memory)
            mov         esi, dword [lbapacket.addr]
            xor         ecx, ecx
            mov         cx, word [origcount]
            shl         ecx, 7
            repnz       movsd
            pop         edi
@@:         pop         esi
            pop         ecx
            pop         eax
            ret

prot_hex2bin:
            xor         eax, eax
            xor         ebx, ebx
@@:         mov         bl, byte [esi]
            cmp         bl, '0'
            jb          @f
            cmp         bl, '9'
            jbe         .num
            cmp         bl, 'A'
            jb          @f
            cmp         bl, 'F'
            ja          @f
            sub         bl, 7
.num:       sub         bl, '0'
            shl         eax, 4
            add         eax, ebx
            inc         esi
            dec         cx
            jnz         @b
@@:         ret

prot_dec2bin:
            xor         eax, eax
            xor         ebx, ebx
            xor         edx, edx
            mov         ecx, 10
@@:         mov         bl, byte [esi]
            cmp         bl, '0'
            jb          @f
            cmp         bl, '9'
            ja          @f
            mul         ecx
            sub         bl, '0'
            add         eax, ebx
            inc         esi
            jmp         @b
@@:         ret

;IN: eax=str ptr, ecx=length OUT: eax=num
prot_oct2bin:
            push        ebx
            push        edx
            mov         ebx, eax
            xor         eax, eax
            xor         edx, edx
@@:         shl         eax, 3
            mov         dl, byte[ebx]
            sub         dl, '0'
            add         eax, edx
            inc         ebx
            dec         ecx
            jnz         @b
            pop         edx
            pop         ebx
            ret

protmode_start:
            mov         ax, DATA_PROT
            mov         ds, ax
            mov         es, ax
            mov         fs, ax
            mov         gs, ax
            mov         ss, ax
            mov         esp, 7C00h

            ; ------- Locate initrd --------
            cmp         byte [hasinitrd], 0
            jnz         .initrdrom
            mov         esi, 0C8000h
.nextrom:   cmp         word [esi], 0AA55h
            jne         @f
            cmp         dword [esi+8], 'INIT'
            jne         @f
            cmp         word [esi+12], 'RD'
            jne         @f
            mov         eax, dword [esi+16]
            mov         dword [bootboot.initrd_size], eax
            add         esi, 32
            jmp         .initrdrom
@@:         add         esi, 2048
            cmp         esi, 0F4000h
            jb          .nextrom

            ; read GPT
.getgpt:    xor         eax, eax
            xor         edi, edi
            prot_readsector
if DEBUG eq 1
            cmp         byte [iscdrom], 0
            jz          @f
            DBG32       dbg_cdrom
@@:
end if
            mov         esi, 0A000h+512
            cmp         dword [esi], 'EFI '
            je          @f
.nogpt:     mov         esi, nogpt
            jmp         prot_diefunc
@@:         mov         edi, 0B000h
            mov         ebx, edi
            mov         ecx, 896
            repnz       movsd
            mov         esi, ebx
            mov         ecx, dword [esi+80]     ;number of entries
            mov         ebx, dword [esi+84]     ;size of one entry
            add         esi, 512
            mov         edx, esi                ;first entry
            mov         dword [gpt_ent], ebx
            mov         dword [gpt_num], ecx
            mov         dword [gpt_ptr], edx
            ; first, look for a partition with bootable flag
            mov         esi, edx
@@:         cmp         dword [esi], 0          ;failsafe, jump to parttype search
            jne         .notz
            cmp         dword [esi+32], 0
            jz          .nextgpt
.notz:      bt          word [esi+48], 2        ;EFI_PART_USED_BY_OS?
            jc          .loadesp2
            add         esi, ebx
            dec         ecx
            jnz         @b
            ; if none, look for specific partition types
.nextgpt:   mov         esi, dword [gpt_ptr]
            mov         ebx, dword [gpt_ent]
            mov         ecx, dword [gpt_num]
            mov         eax, 0C12A7328h
            mov         edx, 011D2F81Fh
@@:         cmp         dword [esi], eax        ;GUID match?
            jne         .note
            cmp         dword [esi+4], edx
            je          .loadesp
.note:      cmp         dword [esi], 'OS/Z'     ;or OS/Z root partition for this architecture?
            jne         .noto
            cmp         word [esi+4], 08664h
            jne         .noto
            cmp         dword [esi+12], 'root'
            je          .loadesp
.noto:      add         esi, ebx
            dec         ecx
            jnz         @b
.nopart:    mov         esi, nopar
            jmp         prot_diefunc

            ; load ESP at free memory hole found
.loadesp:   mov         dword [gpt_ptr], esi
            mov         dword [gpt_num], ecx
.loadesp2:  mov         ecx, dword [esi+40]     ;last sector
            mov         eax, dword [esi+32]     ;first sector
            mov         edx, dword [esi+36]
            or          edx, edx
            jnz         .nextgpt
            or          ecx, ecx
            jz          .nextgpt
            or          eax, eax
            jz          .nextgpt
            mov         dword [bpb_sec], eax
            ;load BPB
            mov         edi, dword [bootboot.initrd_ptr]
            mov         word [lbapacket.count], 8
            prot_readsector

            ;parse fat on EFI System Partition
@@:         cmp         dword [edi + bpb.fst2], 'FAT3'
            je          .isfat
            cmp         dword [edi + bpb.fst], 'FAT1'
            je          .isfat
            ;no, then it's an initrd on the entire partition
            or          eax, eax
            jz          .nopart
            or          ecx, ecx
            jz          .nopart
            sub         ecx, eax
            shl         ecx, 9
            mov         dword [bootboot.initrd_size], ecx
            ; load INITRD from partition
            dec         ecx
            shr         ecx, 12
            mov         edi, dword [bootboot.initrd_ptr]
@@:         add         edi, 4096
            prot_readsector
            add         eax, 8
            dec         ecx
            jnz         @b
            jmp         .initrdloaded

.isfat:     cmp         word [edi + bpb.bps], 512
            jne         .nextgpt
            ;calculations
            xor         eax, eax
            xor         ebx, ebx
            xor         ecx, ecx
            xor         edx, edx
            mov         bx, word [edi + bpb.spf16]
            or          bx, bx
            jnz         @f
            mov         ebx, dword [edi + bpb.spf32]
@@:         mov         al, byte [edi + bpb.nf]
            mov         cx, word [edi + bpb.rsc]
            ;data_sec = numFat*secPerFat
            mul         ebx
            ;data_sec += reservedSec
            add         eax, ecx
            ;data_sec += ESPsec
            add         eax, dword [bpb_sec]
            mov         dword [data_sec], eax
            mov         dword [root_sec], eax
            xor         eax, eax
            mov         al, byte [edi + bpb.spc]
            mov         dword [clu_sec], eax
            ;FAT16
            ;data_sec += (numRootEnt*32+511)/512
            cmp         word [edi + bpb.spf16], 0
            je          .fat32bpb
            cmp         byte [edi + bpb.fst + 4], '6'
            jne         .nextgpt
            xor         eax, eax
            mov         ax, word [edi + bpb.nr]
            shl         eax, 5
            add         eax, 511
            shr         eax, 9
            add         dword [data_sec], eax
            mov         byte [fattype], 0
            xor         ecx, ecx
            mov         cx, word [edi + bpb.spf16]
            jmp         .loadfat
.fat32bpb:  ;FAT32
            ;root_sec += (rootCluster-2)*secPerCluster
            mov         eax, dword [edi + bpb.rc]
            sub         eax, 2
            xor         edx, edx
            mov         ebx, dword [clu_sec]
            mul         ebx
            add         dword [root_sec], eax
            mov         byte [fattype], 1
            mov         ecx, dword [edi + bpb.spf32]
            ;load FAT
.loadfat:   xor         eax, eax
            mov         ax, word [edi+bpb.rsc]
            add         eax, dword [bpb_sec]
            shr         ecx, 3
            inc         ecx
            mov         edi, 0x10000
            mov         word [lbapacket.count], 8
@@:         prot_readsector
            add         edi, 4096
            add         eax, 8
            dec         ecx
            jnz         @b
            mov         ax, word [clu_sec]
            mov         word [lbapacket.count], ax

            ;load root directory
            mov         eax, dword [root_sec]
            mov         edi, dword [bootboot.initrd_ptr]
            prot_readsector

            ;look for BOOTBOOT directory
            mov         esi, edi
            mov         eax, 'BOOT'
            mov         ecx, 255
.nextroot:  cmp         dword [esi], eax
            jne         @f
            cmp         dword [esi+4], eax
            jne         @f
            cmp         word [esi+8], '  '
            je          .foundroot
@@:         add         esi, 32
            cmp         byte [esi], 0
            jz          .nextgpt
            dec         ecx
            jnz         .nextroot
            jmp         .nextgpt
.foundroot: xor         eax, eax
            cmp         byte [fattype], 0
            jz          @f
            mov         ax, word [esi + fatdir.ch]
            shl         eax, 16
@@:         mov         ax, word [esi + fatdir.cl]
            ;sec = (cluster-2)*secPerCluster+data_sec
            sub         eax, 2
            mov         ebx, dword [clu_sec]
            mul         ebx
            add         eax, dword [data_sec]
            mov         edi, dword [bootboot.initrd_ptr]
            prot_readsector

            ;look for CONFIG and INITRD
            mov         esi, edi
            mov         ecx, 255
            mov         edx, dword [bkp]
.nextdir:   cmp         dword [esi], 'CONF'
            jne         .notcfg
            cmp         dword [esi+4], 'IG  '
            jne         .notcfg
            cmp         dword [esi+7], '    '
            jne         .notcfg
            ; load environment from FS0:/BOOTBOOT/CONFIG
            push        edx
            push        esi
            push        edi
            push        ecx
            mov         ecx, dword [esi + fatdir.size]
            cmp         ecx, 4095
            jbe         @f
            mov         ecx, 4095
@@:         mov         dword [core_len], ecx
            xor         eax, eax
            cmp         byte [fattype], 0
            jz          @f
            mov         ax, word [esi + fatdir.ch]
            shl         eax, 16
@@:         mov         ax, word [esi + fatdir.cl]
            mov         edi, 9000h
.nextcfg:   push        eax
            ;sec = (cluster-2)*secPerCluster+data_sec
            sub         eax, 2
            mov         ebx, dword [clu_sec]
            mov         word [lbapacket.count], bx
            mul         ebx
            shl         ebx, 9
            add         eax, dword [data_sec]
            push        ebx
            prot_readsector
            pop         ebx
            pop         eax
            add         edi, ebx
            sub         ecx, ebx
            js          .cfgloaded
            jz          .cfgloaded
            ;get next cluster from FAT
            cmp         byte [fattype], 0
            jz          @f
            shl         eax, 2
            add         eax, 0x10000
            mov         eax, dword [eax]
            jmp         .nextcfg
@@:         shl         eax, 1
            add         eax, 0x10000
            mov         ax, word [eax]
            and         eax, 0FFFFh
            jmp         .nextcfg
.cfgloaded: pop         ecx
            pop         edi
            pop         esi
            pop         edx
            xor         eax, eax
            mov         ecx, 4096
            sub         ecx, dword [core_len]
            mov         edi, 9000h
            add         edi, dword [core_len]
            repnz       stosb
            mov         dword [core_len], eax
            mov         byte [9FFFh], al
            jmp         .notinit
.notcfg:
            cmp         dword [esi], 'X86_'
            jne         @f
            cmp         dword [esi+4], '64  '
            je          .altinitrd
@@:         cmp         dword [esi], 'INIT'
            jne         .notinit
            cmp         dword [esi+4], 'RD  '
            jne         .notinit
            cmp         dword [esi+7], edx
            jne         .notinit

.altinitrd: mov         ecx, dword [esi + fatdir.size]
            mov         dword [bootboot.initrd_size], ecx
            xor         eax, eax
            cmp         byte [fattype], 0
            jz          @f
            mov         ax, word [esi + fatdir.ch]
            shl         eax, 16
@@:         mov         ax, word [esi + fatdir.cl]
            jmp         .loadinitrd

.notinit:   add         esi, 32
            cmp         byte [esi], 0
            jz          .loadinitrd
            dec         ecx
            jnz         .nextdir
.noinitrd:  mov         esi, nord
            jmp         prot_diefunc

            ;load cluster chain, eax=cluster, ecx=size
.loadinitrd:
            mov         edi, dword [bootboot.initrd_ptr]
.nextclu:   push        eax
            ;sec = (cluster-2)*secPerCluster+data_sec
            sub         eax, 2
            mov         ebx, dword [clu_sec]
            mov         word [lbapacket.count], bx
            mul         ebx
            shl         ebx, 9
            add         eax, dword [data_sec]
            push        ebx
            prot_readsector
            pop         ebx
            pop         eax
            add         edi, ebx
            sub         ecx, ebx
            js          .initrdloaded
            jz          .initrdloaded
            ;get next cluster from FAT
            cmp         byte [fattype], 0
            jz          @f
            shl         eax, 2
            add         eax, 0x10000
            mov         eax, dword [eax]
            jmp         .nextclu
@@:         shl         eax, 1
            add         eax, 0x10000
            mov         ax, word [eax]
            and         eax, 0FFFFh
            jmp         .nextclu

.initrdloaded:
            DBG32       dbg_initrd
            mov         esi, dword [bootboot.initrd_ptr]
.initrdrom:
            mov         edi, dword [bootboot.initrd_ptr]
            cmp         word [esi], 08b1fh
            jne         .noinflate
            DBG32       dbg_gzinitrd
            mov         ebx, esi
            mov         eax, dword [bootboot.initrd_size]
            add         ebx, eax
            sub         ebx, 4
            mov         ecx, dword [ebx]
            mov         dword [bootboot.initrd_size], ecx
            add         edi, eax
            add         edi, 4095
            shr         edi, 12
            shl         edi, 12
            add         eax, ecx
            cmp         eax, (INITRD_MAXSIZE+2)*1024*1024
            jb          @f
            mov         esi, nogzmem
            jmp         prot_diefunc
@@:         mov         dword [bootboot.initrd_ptr], edi
            ; inflate initrd
            xor         eax, eax
            add         esi, 2
            lodsb
            cmp         al, 8
            jne         tinf_err
            lodsb
            mov         bl, al
            add         esi, 6
            test        bl, 4
            jz          @f
            lodsw
            add         esi, eax
@@:         test        bl, 8
            jz          .noname
@@:         lodsb
            or          al, al
            jnz         @b
.noname:    test        bl, 16
            jz          .nocmt
@@:         lodsb
            or          al, al
            jnz         @b
.nocmt:     test        bl, 2
            jz          @f
            add         esi, 2
@@:         call        tinf_uncompress
.noinflate:
            ;round up to page size
            mov         eax, dword [bootboot.initrd_size]
            add         eax, 4095
            shr         eax, 12
            shl         eax, 12
            mov         dword [bootboot.initrd_size], eax

            ;do we have an environment configuration?
            mov         ebx, 9000h
            cmp         byte [ebx], 0
            jnz         .parsecfg

            ;-----load /sys/config------
            mov         edx, fsdrivers
.nextfs1:   xor         ebx, ebx
            mov         bx, word [edx]
            or          bx, bx
            jz          .errfs1
            mov         esi, dword [bootboot.initrd_ptr]
            mov         ecx, dword [bootboot.initrd_size]
            add         ecx, esi
            mov         edi, cfgfile
            push        edx
            call        ebx
            pop         edx
            or          ecx, ecx
            jnz         .fscfg
            add         edx, 2
            jmp         .nextfs1
.fscfg:     mov         edi, 9000h
            add         ecx, 3
            shr         ecx, 2
            repnz       movsd
.errfs1:
            ;do we have an environment configuration?
            mov         ebx, 9000h
            cmp         byte [ebx], 0
            jz          .noconf

            ;parse
.parsecfg:  push        ebx
            DBG32       dbg_env
            pop         esi
            jmp         .getnext

            ;skip comments
.nextvar:   cmp         byte[esi], '#'
            je          .skipcom
            cmp         word[esi], '//'
            jne         @f
            add         esi, 2
.skipcom:   lodsb
            cmp         al, 10
            je          .getnext
            cmp         al, 13
            je          .getnext
            or          al, al
            jz          .parseend
            cmp         esi, 0A000h
            ja          .parseend
            jmp         .skipcom
@@:         cmp         word[esi], '/*'
            jne         @f
.skipcom2:  inc         esi
            cmp         word [esi-2], '*/'
            je          .getnext
            cmp         byte [esi], 0
            jz          .parseend
            cmp         esi, 0A000h
            ja          .parseend
            jmp         .skipcom2
            ;parse screen dimensions
@@:         cmp         dword[esi], 'scre'
            jne         @f
            cmp         word[esi+4], 'en'
            jne         @f
            cmp         byte[esi+6], '='
            jne         @f
            add         esi, 7
            call        prot_dec2bin
            mov         dword [reqwidth], eax
            inc         esi
            call        prot_dec2bin
            mov         dword [reqheight], eax
            jmp         .getnext
            ;get kernel's filename
@@:         cmp         dword[esi], 'kern'
            jne         @f
            cmp         word[esi+4], 'el'
            jne         @f
            cmp         byte[esi+6], '='
            jne         @f
            add         esi, 7
            mov         edi, kernel
.copy:      lodsb
            or          al, al
            jz          .copyend
            cmp         al, ' '
            jz          .copyend
            cmp         al, 13
            jbe         .copyend
            cmp         esi, 0A000h
            ja          .copyend
            cmp         edi, loader_end-1
            jae         .copyend
            stosb
            jmp         .copy
.copyend:   xor         al, al
            stosb
            jmp         .getnext
@@:
            inc         esi
            ;failsafe
.getnext:   cmp         esi, 0A000h
            jae         .parseend
            cmp         byte [esi], 0
            je          .parseend
            ;skip white spaces
            cmp         byte [esi], ' '
            je          @b
            cmp         byte [esi], 13
            jbe         @b
            jmp         .nextvar
.noconf:    mov         edi, ebx
            mov         ecx, 1024
            xor         eax, eax
            repnz       stosd
            mov         dword [ebx+0], '// N'
            mov         dword [ebx+4], '/A\n'
.parseend:

            ;-----load /sys/core------
            mov         edx, fsdrivers
.nextfs:    xor         ebx, ebx
            mov         bx, word [edx]
            or          bx, bx
            jz          .errfs
            mov         esi, dword [bootboot.initrd_ptr]
            mov         ecx, dword [bootboot.initrd_size]
            add         ecx, esi
            mov         edi, kernel
            push        edx
            call        ebx
            pop         edx
            or          ecx, ecx
            jnz         .coreok
            add         edx, 2
            jmp         .nextfs
.errfs2:    mov         esi, nocore
            jmp         prot_diefunc
.errfs:     ; if all drivers failed, search for the first elf executable
            DBG32       dbg_scan
            mov         esi, dword [bootboot.initrd_ptr]
            mov         ecx, dword [bootboot.initrd_size]
            add         ecx, esi
            dec         esi
@@:         inc         esi
            cmp         esi, ecx
            jae         .errfs2
            cmp         dword [esi], 5A2F534Fh ; OS/Z magic
            je          .alt
            cmp         dword [esi], 464C457Fh ; ELF magic
            je          .alt
            cmp         word [esi], 5A4Dh      ; MZ magic
            jne         @b
            mov         eax, dword [esi+0x3c]
            cmp         eax, 65536
            jnl         @b
            add         eax, esi
            cmp         dword [eax], 00004550h ; PE magic
            jne         @b
            cmp         word [eax+4], 8664h    ; x86_64
            jne         @b
            cmp         word [eax+20], 20Bh
            jne         @b
.alt:       cmp         word [esi+4], 0102h    ;lsb 64 bit
            jne         @b
            cmp         word [esi+18], 62      ;x86_64
            jne         @b
            cmp         word [esi+0x38], 0     ;e_phnum > 0
            jz          @b
.coreok:
            ; parse PE
            cmp         word [esi], 5A4Dh      ; MZ magic
            jne         .tryelf
            mov         ebx, esi
            cmp         dword [esi+0x3c], 65536
            jnl         .badcore
            add         esi, dword [esi+0x3c]
            cmp         dword [esi], 00004550h ; PE magic
            jne         .badcore
            cmp         word [esi+4], 8664h    ; x86_64 architecture
            jne         .badcore
            cmp         word [esi+20], 20Bh    ; PE32+ format
            jne         .badcore

            DBG32       dbg_pe

            mov         eax, dword [esi+36]    ; entry point
            mov         dword [entrypoint], eax
            mov         dword [entrypoint+4], 0FFFFFFFFh
            mov         ecx, eax
            sub         ecx, dword [esi+40]    ; - code base
            add         ecx, dword [esi+24]    ; + text size
            add         ecx, dword [esi+28]    ; + data size
            mov         edx, dword [esi+32]    ; bss size
            shr         eax, 31
            jz          .badcore
            jmp         .mkcore

            ; parse ELF
.tryelf:    cmp         dword [esi], 5A2F534Fh ; OS/Z magic
            je          @f
            cmp         dword [esi], 464C457Fh ; ELF magic
            jne         .badcore
@@:         cmp         word [esi+4], 0102h    ;lsb 64 bit, little endian
            jne         .badcore
            cmp         word [esi+18], 62      ;x86_64 architecture
            je          @f
.badcore:   mov         esi, badcore
            jmp         prot_diefunc
@@:
            DBG32       dbg_elf

            mov         ebx, esi
            mov         eax, dword [esi+0x18]
            mov         dword [entrypoint], eax
            mov         eax, dword [esi+0x18+4]
            mov         dword [entrypoint+4], eax
            ;parse ELF binary
            mov         cx, word [esi+0x38]     ; program header entries phnum
            mov         eax, dword [esi+0x20]   ; program header
            add         esi, eax
            sub         esi, 56
            inc         cx
.nextph:    add         esi, 56
            dec         cx
            jz          .badcore
            cmp         word [esi], 1               ; p_type, loadable
            jne         .nextph
            cmp         word [esi+22], 0FFFFh       ; p_vaddr == negative address
            jne         .nextph
            ;got it
            add         ebx, dword [esi+8]          ; + P_offset
            mov         ecx, dword [esi+32]         ; p_filesz
            ; hack to keep symtab and strtab for shared libraries
            cmp         byte [ebx+16], 3
            jne         @f
            add         ecx, 04000h
@@:         mov         edx, dword [esi+40]         ; p_memsz
            sub         edx, ecx

            ;ebx=ptr to core segment, ecx=segment size, edx=bss size
.mkcore:    or          ecx, ecx
            jz          .badcore
            mov         edi, dword [bootboot.initrd_ptr]
            add         edi, dword [bootboot.initrd_size]
            mov         dword [core_ptr], edi
            mov         dword [core_len], ecx
            ;copy code from segment after initrd
            mov         esi, ebx
            mov         ebx, ecx
            and         bl, 3
            shr         ecx, 2
            repnz       movsd
            or          bl, bl
            jz          @f
            mov         cl, bl
            repnz       movsb
            ;zero out bss
@@:         or          edx, edx
            jz          @f
            add         dword [core_len], edx
            xor         eax, eax
            mov         ecx, edx
            and         dl, 3
            shr         ecx, 2
            repnz       stosd
            or          dl, dl
            jz          @f
            mov         cl, dl
            repnz       stosb
            ;round up to page size
@@:         mov         eax, dword [core_len]
            add         eax, 4095
            shr         eax, 12
            shl         eax, 12
            mov         dword [core_len], eax

            ;exclude initrd area and core from free mmap
            mov         esi, bootboot.mmap
            mov         ebx, dword [bootboot.initrd_ptr]
            mov         edx, dword [core_ptr]
            add         edx, eax
            mov         cx, 248
            ; initrd+core (ebx..edx)
.nextfree:  cmp         dword [esi], ebx
            ja          .excludeok
            mov         eax, dword [esi+8]
            and         al, 0F0h
            add         eax, dword [esi]
            cmp         edx, eax
            ja          .notini
            ;         +--------------+
            ;  #######
            cmp         dword [esi], ebx
            jne         .splitmem
            ; ptr = initrd_ptr+initrd_size+core_len
            mov         dword [esi], edx
            sub         edx, ebx
            and         dl, 0F0h
            ; size -= initrd_size+core_len
            sub         dword [esi+8], edx
            jmp         .excludeok
            ;  +--+       +----------+
            ;      #######
.splitmem:  mov         edi, bootboot.magic
            add         edi, dword [bootboot.size]
            add         dword [bootboot.size], 16
@@:         mov         eax, dword [edi-16]
            mov         dword [edi], eax
            mov         eax, dword [edi-12]
            mov         dword [edi+4], eax
            mov         eax, dword [edi-8]
            mov         dword [edi+8], eax
            mov         eax, dword [edi-4]
            mov         dword [edi+12], eax
            sub         edi, 16
            cmp         edi, esi
            ja          @b
            mov         eax, ebx
            sub         eax, dword [esi]
            sub         dword [esi+24], eax
            inc         al
            mov         dword [esi+8], eax
            mov         dword [esi+16], edx
            sub         edx, ebx
            sub         dword [esi+24], edx
            jmp         .excludeok
.notini:    add         esi, 16
            dec         cx
            jnz         .nextfree
.excludeok:
            ; -------- SMP ---------
            ; clear LAPIC address and logical core id list
            mov         edi, lapic_ptr
            mov         ecx, 512/4+1
            xor         eax, eax
            repnz       stosd
            ; clear flags
            mov         byte [bsp_done], al
            ; try ACPI first
            mov         esi, dword [bootboot.acpi_ptr]
            or          esi, esi
            jz          .trymp
            mov         edx, 4
            cmp         byte [esi], 'X'     ; XSDT has 8 bytes pointers, but we can only access 4G
            jne         @f
            mov         edx, 8
@@:         mov         ecx, dword [esi+4]
            add         esi, 024h
            sub         ecx, 024h
.nextsdt:   mov         ebx, dword [esi]
            add         esi, edx
            cmp         dword [ebx], 'APIC' ; look for MADT
            je          .madt
            sub         ecx, edx
            or          ecx, ecx
            jz          .trymp
            jmp         .nextsdt
            ; MADT found.
.madt:      mov         eax, dword [ebx+24h]
            mov         dword [lapic_ptr], eax  ; madt.lapic_address
            mov         ecx, dword [ebx+4]
            sub         ecx, 2ch
            add         ebx, 2ch
            mov         edi, lapic_ids
.nextmadtentry:
            cmp         word [bootboot.numcores], 256
            jae         .dosmp
            cmp         byte [ebx], 0       ; madt_entry.type: is it a Local APIC Processor?
            jne         @f
            xor         ax, ax
            mov         al, byte [ebx+2]    ; madt_entry.lapicproc.lapicid
            stosw                           ; ACPI table holds 1 byte id, but internally we have 2 bytes
            inc         word [bootboot.numcores]
@@:         xor         eax, eax
            mov         al, byte [ebx+1]    ; madt_entry.size
            or          al, al
            jz          .dosmp
            add         ebx, eax
            sub         ecx, eax
            cmp         ecx, 0
            jg          .nextmadtentry
            jmp         .dosmp


.trymp:     ; in lack of ACPI, try legacy MP structures
            mov         esi, dword [bootboot.mp_ptr]
            or          esi, esi
            jz          .nosmp
            mov         esi, dword [esi+4]
            cmp         dword [esi], 'PCMP'
            jne         .nosmp
            mov         eax, dword [esi+36]     ; pcmp.lapic_address
            mov         dword [lapic_ptr], eax
            mov         cx, word [esi+34]       ; pcmp.numentries
            add         esi, 44                 ; pcmp header length
            mov         edi, lapic_ids
.nextpcmpentry:
            cmp         word [bootboot.numcores], 256
            jae         .dosmp
            cmp         byte [esi], 0       ; pcmp_entry.type: is it a Local APIC Processor?
            jne         @f
            xor         ax, ax
            mov         al, byte [esi+1]    ; pcmp_entry.lapicproc.lapicid
            stosw                           ; PCMP has 1 byte id, but we use 2 bytes internally
            inc         word [bootboot.numcores]
            add         esi, 12
@@:         add         esi, 8
            dec         cx
            jnz         .nextpcmpentry

            ; send IPI and SIPI
.dosmp:     cmp         word [bootboot.numcores], 2
            jb          .nosmp

            DBG32       dbg_smp

            ; relocate AP trampoline
            mov         esi, ap_trampoline
            mov         edi, 7000h
            mov         ecx, (ap_start-ap_trampoline+3)/4
            repnz       movsd

            ; send Broadcast INIT IPI
            mov         esi, dword [lapic_ptr]
            add         esi, 300h
            mov         eax, 0C4500h
            mov         dword [esi], eax
            ; wait 10 millisec
            mov         cx, 10000/15
            in          al, 61h             ; ps2 control port bit 4 is oscillating at 15 usec
            and         al, 10h
            mov         ah, al
@@:         in          al, 61h
            and         al, 10h
            cmp         al, ah
            je          @b
            mov         ah, al
            dec         cx
            jnz         @b

            ; send Broadcast STARTUP IPI
            mov         eax, 0C4607h        ; start at 0700:0000h
            mov         dword [esi], eax

            ; wait 1 millisec (should be 200 microsec minimum)
            mov         cx, 1000/15
            in          al, 61h             ; ps2 control port bit 4 is oscillating at 15 usec
            and         al, 10h
            mov         ah, al
@@:         in          al, 61h
            and         al, 10h
            cmp         al, ah
            je          @b
            mov         ah, al
            dec         cx
            jnz         @b

            mov         eax, 0C4607h        ; second SIPI
            mov         dword [esi], eax

.nosmp:     ; failsafe
            cmp         word [bootboot.numcores], 0
            jnz         @f
            inc         word [bootboot.numcores]
            mov         ax, word [bootboot.bspid]
            mov         word [lapic_ids], ax
@@:         ; remove core stacks from memory map
            xor         eax, eax
            mov         ax, word [bootboot.numcores]
            shl         eax, 10
            mov         edi, bootboot.mmap
            add         dword [edi], eax
            sub         dword [edi+8], eax

            ; ------- set video resolution -------
            prot_realmode

            DBG         dbg_vesa

            xor         ax, ax
            mov         es, ax
            ;get VESA VBE2.0 info
            mov         ax, 4f00h
            mov         di, 0A000h
            mov         dword [di], 'VBE2'
            ;this call requires a big stack
            int         10h
            cmp         ax, 004fh
            je          @f
.viderr:    mov         si, novbe
            jmp         real_diefunc
            ;read dword pointer and copy string to 1st 64k
            ;available video modes
@@:         xor         esi, esi
            xor         edi, edi
            mov         si, word [0A000h+0Eh]
            mov         ax, word [0A000h+10h]
            mov         ds, ax
            xor         ax, ax
            mov         es, ax
            mov         di, 0A000h+400h
            mov         cx, 64
@@:         lodsw
            cmp         ax, 0ffffh
            je          @f
            or          ax, ax
            jz          @f
            stosw
            dec         cx
            jnz         @b
@@:         xor         ax, ax
            stosw
            ;iterate on modes
            mov         si, 0A000h+400h
.nextmode:  mov         di, 0A000h+800h
            xor         ax, ax
            mov         word [0A000h+802h], ax  ; vbe mode
            lodsw
            or          ax, ax
            jz          .viderr
            mov         cx, ax
            mov         ax, 4f01h
            push        bx
            push        cx
            push        si
            push        di
            int         10h
            pop         di
            pop         si
            pop         cx
            pop         bx
            cmp         ax, 004fh
            jne         .viderr
            bts         cx, 13
            bts         cx, 14
            mov         ax, word [0A000h+800h]  ; vbe flags
            and         ax, VBE_MODEFLAGS
            cmp         ax, VBE_MODEFLAGS
            jne         .nextmode
            ;check memory model (direct)
            cmp         byte [0A000h+81bh], 6
            jne         .nextmode
            ;check bpp
            cmp         byte [0A000h+819h], 32
            jne         .nextmode
            ;check min width
            mov         ax, word [reqwidth]
            cmp         ax, 640
            ja          @f
            mov         ax, 640
@@:         cmp         word [0A000h+812h], ax
            jne         .nextmode
            ;check min height
            mov         ax, word [reqheight]
            cmp         ax, 480
            ja          @f
            mov         ax, 480
@@:         cmp         word [0A000h+814h], ax
            jb          .nextmode
            ;match? go no further
.match:     mov         ax, word [0A000h+810h]
            mov         word [bootboot.fb_scanline], ax
            mov         ax, word [0A000h+812h]
            mov         word [bootboot.fb_width], ax
            mov         ax, word [0A000h+814h]
            mov         word [bootboot.fb_height], ax
            mov         eax, dword [0A000h+828h]
            mov         dword [bootboot.fb_ptr], eax
            mov         byte [bootboot.fb_type],FB_ARGB ; blue offset
            cmp         byte [0A000h+824h], 0
            je          @f
            mov         byte [bootboot.fb_type],FB_RGBA
            cmp         byte [0A000h+824h], 8
            je          @f
            mov         byte [bootboot.fb_type],FB_ABGR
            cmp         byte [0A000h+824h], 16
            je          @f
            mov         byte [bootboot.fb_type],FB_BGRA
@@:         ; set video mode
            mov         bx, cx
            bts         bx, 14 ;flat linear
            mov         ax, 4f02h
            int         10h
            cmp         ax, 004fh
            jne         .viderr
            ;no debug output after this point

            ;inform firmware that we're about to leave it's realm
            mov         ax, 0EC00h
            mov         bx, 2 ; 64 bit
            int         15h
            real_protmode

            ; -------- paging ---------
            ;map core at higher half of memory
            ;address 0xffffffffffe00000
            xor         eax, eax
            mov         edi, 0A000h
            mov         ecx, (14000h+256*1024-0A000h)/4
            repnz       stosd

            ;PML4
            mov         edi, 0A000h
            ;pointer to 2M PDPE (first 4G RAM identity mapped)
            mov         dword [edi], 0E003h
            ;pointer to 4k PDPE (core mapped at -2M)
            mov         dword [edi+4096-8], 0B003h

            ;4K PDPE
            mov         edi, 0B000h
            mov         dword [edi+4096-8], 0C003h
            ;4K PDE
            mov         edi, 0C000h+3840
            mov         eax, dword[bootboot.fb_ptr] ;map framebuffer
            mov         al,83h
            mov         ecx, 31
@@:         stosd
            add         edi, 4
            add         eax, 2*1024*1024
            dec         ecx
            jnz         @b
            mov         dword [0C000h+4096-8], 0D003h

            ;4K PT
            mov         dword[0D000h], 08001h   ;map bootboot
            mov         dword[0D008h], 09001h   ;map configuration
            mov         edi, 0D010h
            mov         eax, dword[core_ptr]    ;map core text segment
            inc         eax
            mov         ecx, dword[core_len]
            shr         ecx, 12
            inc         ecx
@@:         stosd
            add         edi, 4
            add         eax, 4096
            dec         ecx
            jnz         @b
            ;map core stacks (one page per 4 cores)
            mov         edi, 0DFF8h
            mov         eax, 014003h
            mov         cx, word [bootboot.numcores]
            add         cx, 3
            shr         cx, 2
@@:         mov         dword[edi], eax
            sub         edi, 8
            add         eax, 1000h
            dec         cx
            jnz         @b

            ;identity mapping
            ;2M PDPE
            mov         edi, 0E000h
            mov         dword [edi], 0F003h
            mov         dword [edi+8], 010003h
            mov         dword [edi+16], 011003h
            mov         dword [edi+24], 012003h
            ;2M PDE
            mov         edi, 0F000h
            xor         eax, eax
            mov         al, 83h
            mov         ecx, 512*  4;G RAM
@@:         stosd
            add         edi, 4
            add         eax, 2*1024*1024
            dec         ecx
            jnz         @b
            ;first 2M mapped by page
            mov         dword [0F000h], 013003h
            mov         edi, 013000h
            mov         eax, 3
            mov         ecx, 512
@@:         stosd
            add         edi, 4
            add         eax, 4096
            dec         ecx
            jnz         @b

            ;generate new 64 bit gdt
            mov         edi, GDT_table+8
            ;8h core code
            xor         eax, eax        ;supervisor mode (ring 0)
            mov         ax, 0FFFFh
            stosd
            mov         eax, 00209800h
            stosd
            ;10h core data
            xor         eax, eax        ;flat data segment
            mov         ax, 0FFFFh
            stosd
            mov         eax, 00809200h
            stosd
            ;18h mandatory tss
            xor         eax, eax        ;required by vt-x
            mov         al, 068h
            stosd
            mov         eax, 00008900h
            stosd
            xor         eax, eax
            stosd
            stosd

            ;Enter long mode
            cli
            mov         al, 0FFh        ;disable PIC
            out         021h, al
            out         0A1h, al
            in          al, 70h         ;disable NMI
            or          al, 80h
            out         70h, al
            ;release AP spinlock
            inc         byte [bsp_done]

            ;don't use stack below this line
longmode_init:
            mov         eax, 1101101000b  ;Set PAE, MCE, PGE; OSFXSR, OSXMMEXCPT (enable SSE)
            mov         cr4, eax
            mov         eax, 0A000h
            mov         cr3, eax
            mov         ecx, 0C0000080h ;EFER MSR
            rdmsr
            or          eax, 100h       ;enable long mode
            wrmsr

            mov         eax, cr0
            and         al, 0FBh        ;clear EM, MP (enable SSE)
            or          eax, 0C0000001h
            mov         cr0, eax        ;enable paging with cache disabled
            lgdt        [GDT_value]     ;read 80 bit address
            jmp         @f
            nop
@@:         jmp         8:@f
            USE64
@@:         xor         eax, eax        ;load long mode segments
            mov         ax, 10h
            mov         ds, ax
            mov         es, ax
            mov         ss, ax
            mov         fs, ax
            mov         gs, ax
            ; find out our lapic id
            mov         eax, 1
            cpuid
            shr         ebx, 24
            mov         edx, ebx
            ; get array index for it
            xor         rbx, rbx
            mov         rsi, lapic_ids
            mov         cx, word [bootboot.numcores]
@@:         lodsw
            cmp         ax, dx
            je          @f
            inc         ebx
            dec         cx
            jnz         @b
            xor         rbx, rbx
@@:         shl         rbx, 10           ; 1k stack for each core

            ; set stack and call _start() in sys/core
            xor         rsp, rsp        ;sp = core_num * -1024
            sub         rsp, rbx
            jmp         qword[entrypoint]
            nop
            nop
            nop
            nop

            USE32
            include     "fs.inc"
            include     "tinf.inc"

            ;encryption support for FS/Z
if FSZ_SUPPORT eq 1
; --- SHA-256 ---
sha_init:   xor         eax, eax
            mov         dword [sha_l], eax
            mov         dword [sha_b], eax
            mov         dword [sha_b+4], eax
            mov         dword [sha_s   ], 06a09e667h
            mov         dword [sha_s+ 4], 0bb67ae85h
            mov         dword [sha_s+ 8], 03c6ef372h
            mov         dword [sha_s+12], 0a54ff53ah
            mov         dword [sha_s+16], 0510e527fh
            mov         dword [sha_s+20], 09b05688ch
            mov         dword [sha_s+24], 01f83d9abh
            mov         dword [sha_s+28], 05be0cd19h
            ret

            ; IN: ebx = buffer, ecx = length
sha_upd:    push        esi
            or          ecx, ecx
            jz          .end
            mov         esi, ebx
            mov         edi, dword [sha_l]
            add         edi, sha_d
            ; for(;len--;d++) {
            ; ctx->d[ctx->l++]=*d;
.next:      movsb
            inc         byte [sha_l]
            ; if(ctx->l==64) {
            cmp         byte [sha_l], 64
            jne         @f
            ; sha256_t(ctx);
            call        sha_final.sha_t
            ; SHA_ADD(ctx->b[0],ctx->b[1],512);
            add         dword [sha_b], 512
            adc         dword [sha_b+4], 0
            ; ctx->l=0;
            mov         byte [sha_l], 0
            sub         edi, 64
            ; }
@@:         dec         ecx
            jnz         .next
.end:       pop         esi
            ret

            ; IN: edi = output buffer
sha_final:  push        esi
            push        edi
            mov         ebx, edi
            ; i=ctx->l; ctx->d[i++]=0x80;
            mov         edi, dword [sha_l]
            mov         ecx, edi
            add         edi, sha_d
            mov         al, 80h
            stosb
            xor         eax, eax
            ; if(ctx->l<56) {while(i<56) ctx->d[i++]=0x00;}
            cmp         cl, 56
            jae         @f
            neg         ecx
            add         ecx, 63
            xor         al, al
            repnz       stosb
            jmp         .padded
@@:         ; else {while(i<64) ctx->d[i++]=0x00;sha256_t(ctx);memset(ctx->d,0,56);}
            stosb
            call        .sha_t
            push        ecx
            mov         ecx, 56/4
            repnz       stosd
            pop         ecx
            inc         cl
            cmp         cl, 64
            jne         @b
.padded:    ; SHA_ADD(ctx->b[0],ctx->b[1],ctx->l*8);
            mov         eax, dword [sha_l]
            shl         eax, 3
            add         dword [sha_b], eax
            adc         dword [sha_b+4], 0
            ; ctx->d[63]=ctx->b[0];ctx->d[62]=ctx->b[0]>>8;ctx->d[61]=ctx->b[0]>>16;ctx->d[60]=ctx->b[0]>>24;
            mov         eax, dword [sha_b]
            bswap       eax
            mov         dword [sha_d+60], eax
            ; ctx->d[59]=ctx->b[1];ctx->d[58]=ctx->b[1]>>8;ctx->d[57]=ctx->b[1]>>16;ctx->d[56]=ctx->b[1]>>24;
            mov         eax, dword [sha_b+4]
            bswap       eax
            mov         dword [sha_d+56], eax
            ; sha256_t(ctx);
            call        .sha_t
            ; for(i=0;i<4;i++) {
            ;   h[i]   =(ctx->s[0]>>(24-i*8)); h[i+4] =(ctx->s[1]>>(24-i*8));
            ;   h[i+8] =(ctx->s[2]>>(24-i*8)); h[i+12]=(ctx->s[3]>>(24-i*8));
            ;   h[i+16]=(ctx->s[4]>>(24-i*8)); h[i+20]=(ctx->s[5]>>(24-i*8));
            ;   h[i+24]=(ctx->s[6]>>(24-i*8)); h[i+28]=(ctx->s[7]>>(24-i*8));
            ; }
            mov         edi, ebx
            mov         esi, sha_s
            mov         cl, 8
@@:         lodsd
            bswap       eax
            stosd
            dec         cl
            jnz         @b
            pop         edi
            pop         esi
            ret
; private func, sha transform
.sha_t:     push        esi
            push        edi
            push        edx
            push        ecx
            push        ebx
            ; for(i=0,j=0;i<16;i++,j+=4) m[i]=(ctx->d[j]<<24)|(ctx->d[j+1]<<16)|(ctx->d[j+2]<<8)|(ctx->d[j+3]);
            mov         cl, 16
            mov         edi, _m
            mov         esi, sha_d
@@:         lodsd
            bswap       eax
            stosd
            dec         cl
            jnz         @b
            ; for(;i<64;i++) m[i]=SHA_SIG1(m[i-2])+m[i-7]+SHA_SIG0(m[i-15])+m[i-16];
            mov         cl, 48
            ;   SHA_SIG0[m[i-15])       (SHA_ROTR(x,7)^SHA_ROTR(x,18)^((x)>>3))
@@:         mov         eax, dword [edi-15*4]
            mov         ebx, eax
            mov         edx, eax
            ror         eax, 7
            ror         ebx, 18
            shr         edx, 3
            xor         eax, ebx
            xor         eax, edx
            ;   SHA_SIG1(m[i-2])        (SHA_ROTR(x,17)^SHA_ROTR(x,19)^((x)>>10))
            mov         ebx, dword [edi-2*4]
            mov         edx, ebx
            ror         ebx, 17
            ror         edx, 19
            xor         ebx, edx
            rol         edx, 19
            shr         edx, 10
            xor         ebx, edx
            add         eax, ebx
            ;   m[i-7]
            add         eax, dword [edi-7*4]
            ;   m[i-16]
            add         eax, dword [edi-16*4]
            stosd
            dec         cl
            jnz         @b
            ; a=ctx->s[0];b=ctx->s[1];c=ctx->s[2];d=ctx->s[3];
            ; e=ctx->s[4];f=ctx->s[5];g=ctx->s[6];h=ctx->s[7];
            xor         ecx, ecx
            mov         cl, 8
            mov         esi, sha_s
            mov         edi, _a
            repnz       movsd
            ; for(i=0;i<64;i++) {
            mov         esi, _m
@@:         ; t1=h+SHA_EP1(e)+SHA_CH(e,f,g)+sha256_k[i]+m[i];
            mov         eax, dword [_h]
            mov         dword [t1], eax
            ;   SHA_EP1(e)              (SHA_ROTR(x,6)^SHA_ROTR(x,11)^SHA_ROTR(x,25))
            mov         eax, dword [_e]
            mov         ebx, eax
            ror         eax, 6
            ror         ebx, 11
            xor         eax, ebx
            ror         ebx, 14 ; 25 = 11+14
            xor         eax, ebx
            add         dword [t1], eax
            ;   SHA_CH(e,f,g)           (((x)&(y))^(~(x)&(z)))
            mov         eax, dword [_e]
            mov         ebx, eax
            not         ebx
            and         eax, dword [_f]
            and         ebx, dword [_g]
            xor         eax, ebx
            add         dword [t1], eax
            ;   sha256_k[i]
            mov         eax, dword [sha256_k+4*ecx]
            add         dword [t1], eax
            ;   m[i]
            lodsd
            add         dword [t1], eax
            ; t2=SHA_EP0(a)+SHA_MAJ(a,b,c);
            ;   SHA_EP0(a)              (SHA_ROTR(x,2)^SHA_ROTR(x,13)^SHA_ROTR(x,22))
            mov         eax, dword [_a]
            mov         ebx, eax
            ror         eax, 2
            ror         ebx, 13
            xor         eax, ebx
            ror         ebx, 9 ; 22 = 13+9
            xor         eax, ebx
            mov         dword [t2], eax
            ;   SHA_MAJ(a,b,c)          (((x)&(y))^((x)&(z))^((y)&(z)))
            mov         eax, dword [_a]
            mov         edx, dword [_c]
            mov         ebx, eax
            and         eax, dword [_b]
            and         ebx, edx
            xor         eax, ebx
            mov         ebx, dword [_b]
            and         ebx, edx
            xor         eax, ebx
            add         dword [t2], eax
            ; h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
            mov         eax, dword [_g]
            mov         dword [_h], eax
            mov         eax, dword [_f]
            mov         dword [_g], eax
            mov         eax, dword [_e]
            mov         dword [_f], eax
            mov         eax, dword [_d]
            add         eax, dword [t1]
            mov         dword [_e], eax
            mov         eax, dword [_c]
            mov         dword [_d], eax
            mov         eax, dword [_b]
            mov         dword [_c], eax
            mov         eax, dword [_a]
            mov         dword [_b], eax
            mov         eax, dword [t1]
            add         eax, dword [t2]
            mov         dword [_a], eax
            ; }
            inc         cl
            cmp         cl, 64
            jne         @b
            ; ctx->s[0]+=a;ctx->s[1]+=b;ctx->s[2]+=c;ctx->s[3]+=d;
            ; ctx->s[4]+=e;ctx->s[5]+=f;ctx->s[6]+=g;ctx->s[7]+=h;
            mov         cl, 8
            mov         esi, _a
            mov         edi, sha_s
@@:         lodsd
            add         dword [edi], eax
            add         edi, 4
            dec         cl
            jnz         @b
            pop         ebx
            pop         ecx
            pop         edx
            pop         edi
            pop         esi
            xor         eax, eax
            ret

; --- CRC-32c ---
            ; IN: esi = buffer, ecx = length
            ; OUT: edx = crc
crc32_calc: xor         edx, edx
            xor         eax, eax
            xor         ebx, ebx
            or          cx, cx
            jz          .end
.next:      lodsb
            mov         bl, dl
            xor         bl, al
            mov         eax, edx
            shr         edx, 8
            xor         edx, dword [crclkp+4*ebx]
            dec         cx
            jnz         .next
.end:       ret
end if

;*********************************************************************
;*                               Data                                *
;*********************************************************************
            ;global descriptor table
            align       16
GDT_table:  dd          0, 0                ;null descriptor
DATA_PROT   =           $-GDT_table
            dd          0000FFFFh,008F9200h ;flat ds
DATA_BOOT   =           $-GDT_table
            dd          0000FFFFh,00009200h ;16 bit legacy real mode ds
CODE_BOOT   =           $-GDT_table
            dd          0000FFFFh,00009800h ;16 bit legacy real mode cs
CODE_PROT   =           $-GDT_table
            dd          0000FFFFh,00CF9A00h ;32 bit prot mode ring0 cs
            dd          00000068h,00CF8900h ;32 bit TSS, not used but required
GDT_value:  dw          $-GDT_table
            dd          GDT_table
            dd          0,0
            ;lookup tables for initrd encryption
if FSZ_SUPPORT eq 1
            dw          0
crclkp:     dd          000000000h, 0F26B8303h, 0E13B70F7h, 01350F3F4h, 0C79A971Fh, 035F1141Ch, 026A1E7E8h, 0D4CA64EBh
            dd          08AD958CFh, 078B2DBCCh, 06BE22838h, 09989AB3Bh, 04D43CFD0h, 0BF284CD3h, 0AC78BF27h, 05E133C24h
            dd          0105EC76Fh, 0E235446Ch, 0F165B798h, 0030E349Bh, 0D7C45070h, 025AFD373h, 036FF2087h, 0C494A384h
            dd          09A879FA0h, 068EC1CA3h, 07BBCEF57h, 089D76C54h, 05D1D08BFh, 0AF768BBCh, 0BC267848h, 04E4DFB4Bh
            dd          020BD8EDEh, 0D2D60DDDh, 0C186FE29h, 033ED7D2Ah, 0E72719C1h, 0154C9AC2h, 0061C6936h, 0F477EA35h
            dd          0AA64D611h, 0580F5512h, 04B5FA6E6h, 0B93425E5h, 06DFE410Eh, 09F95C20Dh, 08CC531F9h, 07EAEB2FAh
            dd          030E349B1h, 0C288CAB2h, 0D1D83946h, 023B3BA45h, 0F779DEAEh, 005125DADh, 01642AE59h, 0E4292D5Ah
            dd          0BA3A117Eh, 04851927Dh, 05B016189h, 0A96AE28Ah, 07DA08661h, 08FCB0562h, 09C9BF696h, 06EF07595h
            dd          0417B1DBCh, 0B3109EBFh, 0A0406D4Bh, 0522BEE48h, 086E18AA3h, 0748A09A0h, 067DAFA54h, 095B17957h
            dd          0CBA24573h, 039C9C670h, 02A993584h, 0D8F2B687h, 00C38D26Ch, 0FE53516Fh, 0ED03A29Bh, 01F682198h
            dd          05125DAD3h, 0A34E59D0h, 0B01EAA24h, 042752927h, 096BF4DCCh, 064D4CECFh, 077843D3Bh, 085EFBE38h
            dd          0DBFC821Ch, 02997011Fh, 03AC7F2EBh, 0C8AC71E8h, 01C661503h, 0EE0D9600h, 0FD5D65F4h, 00F36E6F7h
            dd          061C69362h, 093AD1061h, 080FDE395h, 072966096h, 0A65C047Dh, 05437877Eh, 04767748Ah, 0B50CF789h
            dd          0EB1FCBADh, 0197448AEh, 00A24BB5Ah, 0F84F3859h, 02C855CB2h, 0DEEEDFB1h, 0CDBE2C45h, 03FD5AF46h
            dd          07198540Dh, 083F3D70Eh, 090A324FAh, 062C8A7F9h, 0B602C312h, 044694011h, 05739B3E5h, 0A55230E6h
            dd          0FB410CC2h, 0092A8FC1h, 01A7A7C35h, 0E811FF36h, 03CDB9BDDh, 0CEB018DEh, 0DDE0EB2Ah, 02F8B6829h
            dd          082F63B78h, 0709DB87Bh, 063CD4B8Fh, 091A6C88Ch, 0456CAC67h, 0B7072F64h, 0A457DC90h, 0563C5F93h
            dd          0082F63B7h, 0FA44E0B4h, 0E9141340h, 01B7F9043h, 0CFB5F4A8h, 03DDE77ABh, 02E8E845Fh, 0DCE5075Ch
            dd          092A8FC17h, 060C37F14h, 073938CE0h, 081F80FE3h, 055326B08h, 0A759E80Bh, 0B4091BFFh, 0466298FCh
            dd          01871A4D8h, 0EA1A27DBh, 0F94AD42Fh, 00B21572Ch, 0DFEB33C7h, 02D80B0C4h, 03ED04330h, 0CCBBC033h
            dd          0A24BB5A6h, 0502036A5h, 04370C551h, 0B11B4652h, 065D122B9h, 097BAA1BAh, 084EA524Eh, 07681D14Dh
            dd          02892ED69h, 0DAF96E6Ah, 0C9A99D9Eh, 03BC21E9Dh, 0EF087A76h, 01D63F975h, 00E330A81h, 0FC588982h
            dd          0B21572C9h, 0407EF1CAh, 0532E023Eh, 0A145813Dh, 0758FE5D6h, 087E466D5h, 094B49521h, 066DF1622h
            dd          038CC2A06h, 0CAA7A905h, 0D9F75AF1h, 02B9CD9F2h, 0FF56BD19h, 00D3D3E1Ah, 01E6DCDEEh, 0EC064EEDh
            dd          0C38D26C4h, 031E6A5C7h, 022B65633h, 0D0DDD530h, 00417B1DBh, 0F67C32D8h, 0E52CC12Ch, 01747422Fh
            dd          049547E0Bh, 0BB3FFD08h, 0A86F0EFCh, 05A048DFFh, 08ECEE914h, 07CA56A17h, 06FF599E3h, 09D9E1AE0h
            dd          0D3D3E1ABh, 021B862A8h, 032E8915Ch, 0C083125Fh, 0144976B4h, 0E622F5B7h, 0F5720643h, 007198540h
            dd          0590AB964h, 0AB613A67h, 0B831C993h, 04A5A4A90h, 09E902E7Bh, 06CFBAD78h, 07FAB5E8Ch, 08DC0DD8Fh
            dd          0E330A81Ah, 0115B2B19h, 0020BD8EDh, 0F0605BEEh, 024AA3F05h, 0D6C1BC06h, 0C5914FF2h, 037FACCF1h
            dd          069E9F0D5h, 09B8273D6h, 088D28022h, 07AB90321h, 0AE7367CAh, 05C18E4C9h, 04F48173Dh, 0BD23943Eh
            dd          0F36E6F75h, 00105EC76h, 012551F82h, 0E03E9C81h, 034F4F86Ah, 0C69F7B69h, 0D5CF889Dh, 027A40B9Eh
            dd          079B737BAh, 08BDCB4B9h, 0988C474Dh, 06AE7C44Eh, 0BE2DA0A5h, 04C4623A6h, 05F16D052h, 0AD7D5351h

sha256_k:   dd          0428a2f98h, 071374491h, 0b5c0fbcfh, 0e9b5dba5h, 03956c25bh, 059f111f1h, 0923f82a4h, 0ab1c5ed5h
            dd          0d807aa98h, 012835b01h, 0243185beh, 0550c7dc3h, 072be5d74h, 080deb1feh, 09bdc06a7h, 0c19bf174h
            dd          0e49b69c1h, 0efbe4786h, 00fc19dc6h, 0240ca1cch, 02de92c6fh, 04a7484aah, 05cb0a9dch, 076f988dah
            dd          0983e5152h, 0a831c66dh, 0b00327c8h, 0bf597fc7h, 0c6e00bf3h, 0d5a79147h, 006ca6351h, 014292967h
            dd          027b70a85h, 02e1b2138h, 04d2c6dfch, 053380d13h, 0650a7354h, 0766a0abbh, 081c2c92eh, 092722c85h
            dd          0a2bfe8a1h, 0a81a664bh, 0c24b8b70h, 0c76c51a3h, 0d192e819h, 0d6990624h, 0f40e3585h, 0106aa070h
            dd          019a4c116h, 01e376c08h, 02748774ch, 034b0bcb5h, 0391c0cb3h, 04ed8aa4ah, 05b9cca4fh, 0682e6ff3h
            dd          0748f82eeh, 078a5636fh, 084c87814h, 08cc70208h, 090befffah, 0a4506cebh, 0bef9a3f7h, 0c67178f2h
end if
idt16:      dw          3FFh
            dq          0
lbapacket:              ;lba packet for BIOS
.size:      dw          10h
.count:     dw          8
.addr:      dd          0A000h
.sect0:     dd          0
.sect1:     dd          0
spc_packet: db          18h dup 0
reqwidth:   dd          1024
reqheight:  dd          768
ebdaptr:    dd          0
hw_stack:   dd          0
bpb_sec:    dd          0 ;ESP's first sector
root_sec:   dd          0 ;root directory's first sector
data_sec:   dd          0 ;first data sector
clu_sec:    dd          0 ;sector per cluster
origcount:  dw          0
bootdev:    db          0
readdev:    db          0
hasinitrd:  db          0
hasconfig:  db          0
iscdrom:    db          0
bsp_done:                 ;flag to indicate APs can run
fattype:    db          0
bkp:        dd          '    '
if DEBUG eq 1
dbg_cpu     db          " * Detecting CPU",10,13,0
dbg_A20     db          " * Enabling A20",10,13,0
dbg_mem     db          " * E820 Memory Map",10,13,0
dbg_systab  db          " * System tables",10,13,0
dbg_time    db          " * System time",10,13,0
dbg_cdrom   db          " * Detected CDROM boot",10,13,0
dbg_env     db          " * Environment",10,13,0
dbg_initrd  db          " * Initrd loaded",10,13,0
dbg_gzinitrd db         " * Gzip compressed initrd",10,13,0
dbg_scan    db          " * Autodetecting kernel",10,13,0
dbg_elf     db          " * Parsing ELF64",10,13,0
dbg_pe      db          " * Parsing PE32+",10,13,0
dbg_smp     db          " * SMP init",10,13,0
dbg_vesa    db          " * Screen VESA VBE",10,13,0
end if
backup:     db          " * Backup initrd",10,13,0
passphrase: db          " * Passphrase? ",0
decrypting: db          13," * Decrypting...",0
clrdecrypt: db          13,"                ",13,0
starting:   db          "Booting OS...",10,13,0
panic:      db          "-PANIC: ",0
noarch:     db          "Hardware not supported",0
a20err:     db          "Failed to enable A20",0
memerr:     db          "E820 memory map not found",0
nogzmem:    db          "Inflating: "
noenmem:    db          "Not enough memory",0
noacpi:     db          "ACPI not found",0
nogpt:      db          "No GPT found",0
nopar:      db          "No boot partition",0
nord:       db          "Initrd not found",0
nolib:      db          "/sys not found in initrd",0
nocore:     db          "Kernel not found in initrd",0
badcore:    db          "Kernel is not a valid executable",0
novbe:      db          "VESA VBE error, no framebuffer",0
nogzip:     db          "Unable to uncompress",0
notcdsect:  db          "Not 2048 sector aligned",0
nocipher:   db          "Unsupported cipher",10,13,0
badpass:    db          13,"BOOTBOOT-ERROR: Bad passphrase",10,13,0
cfgfile:    db          "sys/config",0,0,0
kernel:     db          "sys/core"
            db          (64-($-kernel)) dup 0
;-----------padding to be multiple of 512----------
            db          (511-($-loader+511) mod 512) dup 0
loader_end:

;-----------BIOS checksum------------
chksum = 0
repeat $-loader
    load b byte from (loader+%-1)
    chksum = (chksum + b) mod 100h
end repeat
store byte (100h-chksum) at (loader.checksum)

;-----------bss area-----------
entrypoint: dq          ?
core_ptr:   dd          ?
core_len:   dd          ?
gpt_ptr:    dd          ?
gpt_num:    dd          ?
gpt_ent:    dd          ?
lapic_ptr:  dd          ?
lapic_ids:
tinf_bss_start:
d_end:      dd          ?
d_lzOff:    dd          ?
d_dict_ring:dd          ?
d_dict_size:dd          ?
d_dict_idx: dd          ?
d_tag:      db          ?
d_bitcount: db          ?
d_bfinal:   db          ?
d_curlen:   dw          ?
d_ltree:
d_ltree_table:
            dw          16 dup ?
d_ltree_trans:
            dw          288 dup ?
d_dtree:
d_dtree_table:
            dw          16 dup ?
d_dtree_trans:
            dw          288 dup ?
offs:       dw          16 dup ?
num:        dw          ?
lengths:    db          320 dup ?
hlit:       dw          ?
hdist:      dw          ?
hclen:      dw          ?
tinf_bss_end:

if FSZ_SUPPORT eq 1
virtual at tinf_bss_start
pass:       db          256 dup ?
sha_d:      db          64 dup ?
sha_l:      dd          ?
sha_b:      dd          2 dup ?
sha_s:      dd          8 dup ?
_a:         dd          ?
_b:         dd          ?
_c:         dd          ?
_d:         dd          ?
_e:         dd          ?
_f:         dd          ?
_g:         dd          ?
_h:         dd          ?
t1:         dd          ?
t2:         dd          ?
_m:         dd          64 dup ?
chk:        db          32 dup ?
iv:         db          32 dup ?
pl:         dd          ?
_i:         dd          ?
end virtual
end if

;-----------bound check-------------
;fasm will generate an error if the code
;is bigger than it should be
db  07C00h-4096-($-loader) dup ?
