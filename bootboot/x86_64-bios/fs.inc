;*
;* x86_64-bios/fs.inc
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
;* @brief Filesystem drivers for initial ramdisk.
;*

FSZ_SUPPORT equ 1

;*********************************************************************
;*                       File System Drivers                         *
;*********************************************************************

            USE32
fsdrivers:
if FSZ_SUPPORT eq 1
            dw          fsz_initrd
end if
            dw          cpio_initrd
            dw          tar_initrd
            dw          sfs_initrd
            dw          jamesm_initrd
            dw          0

if FSZ_SUPPORT eq 1
; ----------- FS/Z ----------
; Find the kernel on initrd (only supports 4096 logical sector sizes)
; IN:   esi: initrd pointer, ecx: initrd end, edi: kernel filename
; OUT:  On Success
;         esi: pointer to the first byte, ecx: size in bytes
;       On Error
;         ecx: 0
fsz_initrd:
            mov         ebx, ecx
            xor         ecx, ecx
            ; FS/Z superblock
            cmp         dword [esi+512], 'FS/Z' ; FSZ_SuperBlock.magic
            jne         .nolib
            ; encrypted initrd?
            cmp         dword [esi+708], 0      ; FSZ_SuperBlock.enchash
            jz          .noenc
            mov         al, byte [esi+518]      ; FSZ_SuperBlock.flags
            shr         al, 2
            and         al, 7
            or          al, al
            jz          @f
            prot_realmode
            real_print  loader.name
            real_print  panic
            mov         esi, nocipher
            call        real_printfunc
            real_protmode
            jmp         .err
@@:         push        edi
            prot_realmode
.passagain: real_print  passphrase
            ; get passphrase from user
            mov         di, pass
            mov         byte [di], 0
.getchar:   call        real_getchar
            cmp         al, 27              ; Esc
            jne         @f
            real_print  clrdecrypt
            jmp         .err
@@:         cmp         al, 8               ; Backspace
            jne         @f
            cmp         di, pass
            je          .getchar
            mov         byte [di], 0
            dec         di
            jmp         .getchar
@@:         cmp         al, 13              ; Enter
            je          .gotpass
            cmp         al, 10
            je          .gotpass
            cmp         al, ' '
            jb          .getchar
            cmp         di, pass+255
            jge         .getchar
            mov         word [di], ax
            inc         di
            jmp         .getchar
.gotpass:   push        esi
            real_protmode
            mov         esi, pass
            mov         ecx, edi
            sub         ecx, esi
            mov         dword [pl], ecx
            call        crc32_calc
            prot_realmode
            pop         esi
            cmp         dword [esi+708], edx
            je          .passok
            real_print  badpass
            jmp         .passagain
.passok:    real_print  decrypting
            real_protmode
            ; decrypt initrd
            call        sha_init
            mov         ecx, dword [pl]
            mov         ebx, pass
            call        sha_upd
            mov         ecx, 6
            mov         ebx, esi
            add         ebx, 512                ; FSZ_SuperBlock.magic
            call        sha_upd
            mov         edi, chk
            call        sha_final
            mov         edi, esi
            add         edi, 680                ; FSZ_SuperBlock.encrypt
            mov         cl, 28
            xor         ebx, ebx
@@:         mov         al, byte [edi]
            xor         byte [chk+ebx], al
            xor         eax, eax
            stosb
            inc         ebx
            dec         cl
            jnz         @b
            stosd
            call        sha_init
            mov         ecx, 28
            mov         ebx, chk
            call        sha_upd
            mov         edi, iv
            call        sha_final
            mov         eax, dword [esi+528]    ; FSZ_SuperBlock.numsec
            mov         dword [pl], eax
            xor         eax, eax
            inc         eax
            mov         dword [_i], eax         ; skip first sector
            mov         ebx, esi
            add         ebx, 4096
            push        esi
.decrypt:   mov         esi, iv
            mov         edi, chk
            xor         ecx, ecx
            mov         cl, 32/4
            repnz       movsd
            mov         cx, 4096
.nextblk:   mov         al, bl
            and         al, 31
            jnz         @f
            push        ebx
            push        ecx
            call        sha_init
            mov         ecx, 32
            mov         ebx, chk
            call        sha_upd
            mov         ecx, 4
            mov         ebx, _i
            call        sha_upd
            mov         edi, chk
            call        sha_final
            pop         ecx
            pop         ebx
            mov         edx, edi
@@:         mov         al, byte [edx]
            xor         byte [ebx], al
            mov         al, byte [edx+32]
            xor         byte [ebx], al
            inc         ebx
            inc         edx
            dec         cx
            jnz         .nextblk
            inc         dword [_i]
            mov         eax, dword [_i]
            cmp         eax, dword [pl]
            jne         .decrypt
            mov         esi, dword [esp]
            add         esi, 512
            mov         ecx, 508
            call        crc32_calc
            pop         esi
            mov         dword [esi+1020], edx   ; FSZ_SuperBlock.chksum
            ; clear console message
            prot_realmode
            real_print  clrdecrypt
            real_protmode
            pop         edi

            ; get root dir inode
.noenc:     mov         dword [_i], 1024
            mov         al, byte [esi+518]      ; FSZ_SuperBlock.flags
            bt          ax, 0                   ; FSZ_FLAG_BIGINODE?
            jnc         @f
            mov         dword [_i], 2048
@@:         mov         eax, dword [esi+560]    ; FSZ_SuperBlock.rootdirfid
            shl         eax, 12
            add         esi, eax
            cmp         dword [esi], 'FSIN'
            je          @f
.nolib:     mov         esi, nolib
.err:       xor         ecx, ecx
            ret
.nocore:    mov         esi, nocore
            jmp         .err
@@:         ; it has inlined data?
.again:     mov         eax, dword [esi+448]        ; FSZ_Inode.sec
            add         esi, dword[_i]              ; FSZ_Inode.[big|small].inlinedata
            cmp         dword [esi], 'FSDR'
            je          .srchdir
            ; no, locate the data
            mov         ecx, dword [esi]
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            cmp         dword [esi], 'FSDR'
            je          .srchdir
            ; inlined sector directory or list?
            shl         ecx, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, ecx
            cmp         dword [esi], 'FSDR'
            jne         .nolib
.srchdir:   ; find sys/
            mov         ecx, dword [esi+16]          ; FSZ_DirEntHeader.numentries
            mov         eax, dword [edi]
@@:         add         esi, 128                    ; directories than
            cmp         dword [esi+17], eax
            je          @f
            dec         ecx
            jnz         @b
            jmp         .nolib
            ; found, get it's inode
@@:
            mov         eax, dword [esi]
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            cmp         dword [esi], 'FSIN'
            jne         .nolib

            ;this is not bullet proof
            add         edi, 4
            cmp         byte [edi+3], '/'
            je          .again

            ; it has inlined data?
            mov         eax, dword [esi+448]        ; FSZ_Inode.sec
            add         esi, dword[_i]              ; FSZ_Inode.[big|small].inlinedata
            cmp         dword [esi], 'FSDR'
            je          .srchcore
            ; no, locate the data
            mov         ecx, dword [esi]
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            cmp         dword [esi], 'FSDR'
            je          .srchdir
            ; inlined sector directory or list?
            shl         ecx, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, ecx
            cmp         dword [esi], 'FSDR'
            jne         .nolib

.srchcore:  ; find filename
            mov         ecx, dword [esi+16]          ; FSZ_DirEntHeader.numentries
            ;filename, 8 characters supported
            mov         eax, dword [edi]
            mov         edx, dword [edi+4]
@@:         add         esi, 128
            cmp         dword [esi+21], edx
            jne         .not
            cmp         dword [esi+17], eax
            je          @f
.not:       dec         ecx
            jnz         @b
            jmp         .nocore
            ; found, get it's inode
@@:         mov         eax, dword [esi]
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            cmp         dword [esi], 'FSIN'
            jne         .nocore
            ; get data
            mov         eax, dword [esi+448]    ; FSZ_Inode.sec
            mov         ecx, dword [esi+464]    ; FSZ_Inode.size
            mov         bl, byte [esi+488]      ; FSZ_Inode.flags

            ; inline
            cmp         bl, 0FFh                ; FSZ_IN_FLAG_INLINE
            jne          @f
            add         esi, dword[_i]          ; FSZ_Inode.[big|small].inlinedata
            ret
            ; direct data block
@@:         or          bl, bl                  ; FSZ_IN_FLAG_DIRECT
            je          .load
            ; inlined sector directory or sector list
@@:         cmp         bl, 07Fh                ; FSZ_IN_FLAG_SDINLINE
            je          @f
            cmp         bl, 080h                ; FSZ_IN_FLAG_SECLIST
            je          @f
            cmp         bl, 1                   ; FSZ_IN_FLAG_SD
            jne         .nocore
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            mov         eax, dword [esi]        ; first FSZ_SectorList.sec
            jmp         .load
@@:         add         esi, dword[_i]          ; FSZ_Inode.[big|small].inlinedata
            ; sector directory at esi, file size in ecx
            mov         eax, dword [esi]        ; first FSZ_SectorList.sec
.load:      shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            ret
end if

; ----------- cpio ----------
; Find the kernel on initrd
; IN:   esi: initrd pointer, ecx: initrd end, edi: kernel filename
; OUT:  On Success
;         esi: pointer to the first byte, ecx: size in bytes
;       On Error
;         ecx: 0
cpio_initrd:
            ; upper bound
            mov         ebx, ecx
            xor         ecx, ecx
            ; strlen(kernel)
            mov         eax, edi
            or          eax, eax
            jz          .err
            cmp         byte [eax], 0
            jz          .err
            xor         ecx, ecx
@@:         inc         ecx
            inc         eax
            cmp         byte [eax], 0
            jnz         @b
            mov         dword [.ks], ecx
            ; while(ptr.magic=='070707' && ptr<limit)
.next:      cmp         esi, ebx
            jae         .err
            mov         eax, '0707'
            cmp         dword [esi], eax    ; cpio magic
            jne         .err
            cmp         word [esi+4], ax    ; hpodc
            je          @f
            cmp         word [esi+4], '01'  ; newc
            je          .newc
            cmp         word [esi+4], '02'  ; crc
            je          .newc
.err:       xor         ecx, ecx
            ret
@@:         mov         eax, esi            ; filename len
            add         eax, 8*6+11
            mov         ecx, 6
            call        prot_oct2bin
            mov         dword [.ns], eax
            mov         eax, esi            ; filesize
            add         eax, 8*6+11+6
            mov         ecx, 11
            call        prot_oct2bin
            mov         dword [.fs], eax
            push        esi                 ; name equals?
            push        edi
            add         esi, 9*6+2*11
            mov         ecx, dword [.ks]
            repz        cmpsb
            pop         edi
            pop         esi
            jz          @f
            add         esi, 76             ; no skip this record
            add         esi, dword [.ns]    ; and check the next one
            add         esi, dword [.fs]
            jmp         .next
@@:         add         esi, 76             ; found! esi=data
            add         esi, dword [.ns]
            mov         ecx, dword [.fs]    ; ecx=size
            ret
.newc:      mov         edx, esi            ; filename len
            add         esi, 8*11+6
            mov         ecx, 8
            call        prot_hex2bin
            mov         dword [.ns], eax
            mov         esi, edx            ; filesize
            add         esi, 8*6+6
            mov         ecx, 8
            call        prot_hex2bin
            mov         dword [.fs], eax
            mov         esi, edx
            push        esi                 ; name equals?
            push        edi
            add         esi, 110
            mov         ecx, dword [.ks]
            repz        cmpsb
            pop         edi
            pop         esi
            jz          @f
            mov         eax, 113            ; no skip this record
            add         eax, dword [.ns]    ; and check the next one
            and         al, 0FCh
            add         esi, eax
            mov         eax, dword [.fs]
            add         eax, 3
            and         al, 0FCh
            add         esi, eax
            cmp         dword [esi], '0707' ; cpio magic
            jne         .err
            jmp         .newc
@@:         mov         eax, 113            ; found! esi=data
            add         eax, dword [.ns]
            and         al, 0FCh
            add         esi, eax
            mov         ecx, dword [.fs]    ; ecx=size
            ret
.ks:        dd          0
.ns:        dd          0
.fs:        dd          0

; ----------- tar ----------
; Find the kernel on initrd
; IN:   esi: initrd pointer, ecx: initrd end, edi: kernel filename
; OUT:  On Success
;         esi: pointer to the first byte, ecx: size in bytes
;       On Error
;         ecx: 0
tar_initrd:
            ; upper bound
            mov         ebx, ecx
            xor         ecx, ecx
            ; strlen(kernel)
            mov         eax, edi
            or          eax, eax
            jz          .err
            cmp         byte [eax], 0
            jz          .err
            xor         ecx, ecx
@@:         inc         ecx
            inc         eax
            cmp         byte [eax], 0
            jnz         @b
            mov         dword [.ks], ecx
            ; while(ptr.magic=='ustar' && ptr<limit)
.next:      cmp         esi, ebx
            jae         .err
            cmp         dword [esi+257], 'usta' ; tar magic
            jne         .err
            cmp         byte [esi+261], 'r'     ; tar magic
            je          @f
.err:       xor         ecx, ecx
            ret
@@:         mov         eax, esi            ; filesize
            add         eax, 07ch
            mov         ecx, 11
            call        prot_oct2bin
            mov         dword [.fs], eax
            push        esi                 ; name equals?
            push        edi
            mov         ecx, dword [.ks]
            repz        cmpsb
            pop         edi
            pop         esi
            jz          @f
            add         esi, 512            ; no skip this record
            mov         eax, dword [.fs]    ; and check the next one
            add         eax, 511
            shr         eax, 9
            shl         eax, 9
            add         esi, eax
            jmp         .next
@@:         add         esi, 512            ; found! esi=data
            mov         ecx, dword [.fs]    ; ecx=size
            ret
.ks:        dd          0
.fs:        dd          0

; ----------- SFS ----------
; Find the kernel on Stupid File System
; IN:   esi: initrd pointer, ecx: initrd end, edi: kernel filename
; OUT:  On Success
;         esi: pointer to the first byte, ecx: size in bytes
;       On Error
;         ecx: 0
sfs_initrd:
            ; check magic
            ; 1.0, Brendan's version
            mov         byte [.ver], 0
            cmp         word [esi+01ACh], 'SF'
            jne         @f
            cmp         byte [esi+01AEh], 'S'
            je          .ok
            ; 1.1, BenLunt's version
@@:         cmp         word [esi+01A6h], 'SF'
            jne         .err
            cmp         byte [esi+01A8h], 'S'
            jne         .err
            inc         byte [.ver]

            ; upper bound
.ok:        mov         ebx, ecx
            xor         ecx, ecx
            ; strlen(kernel)
            mov         eax, edi
            or          eax, eax
            jz          .err
            cmp         byte [eax], 0
            jz          .err
            xor         ecx, ecx
@@:         inc         ecx
            inc         eax
            cmp         byte [eax], 0
            jnz         @b
            mov         dword [.ks], ecx
            ; get block size
            xor         eax, eax
            inc         eax
            xor         ecx, ecx
            mov         cl, byte [esi+01BCh]
            cmp         byte [.ver], 0
            jz          @f
            mov         cl, byte [esi+01B6h]
@@:         add         cl, 7
            shl         eax, cl
            mov         dword [.bs], ecx
            ; get index area, base + totalblocks*blocksize - indexsize
            xor         edx, edx
            mov         eax, dword [esi+01B0h]  ; total number of blocks
            cmp         byte [.ver], 0
            jz          @f
            mov         eax, dword [esi+01AAh]  ; total number of blocks
@@:         mul         ecx
            add         eax, esi
            mov         ebx, eax
            mov         edx, dword [esi+01A4h]  ; size of index area
            cmp         byte [.ver], 0
            jz          @f
            mov         edx, dword [esi+019Eh]  ; size of index area
@@:         sub         eax, edx
            mov         edx, esi
            mov         esi, eax
            cmp         byte [esi], 02h         ; got Starting Marker Entry?
            jne         .err
            ; iterate on index until we reach end or Volume Identifier
.nextindex: cmp         esi, ebx
            jae         .err
            cmp         byte [esi], 01h
            je          .err
            add         esi, 64
            cmp         byte [esi], 12h         ; file entry?
            jne         .nextindex
            push        esi                     ; name equals?
            push        edi
            mov         ecx, dword [.ks]
            add         esi, 022h
            add         esi, dword [.ver]
            repz        cmpsb
            pop         edi
            pop         esi
            jnz         .nextindex
            mov         ebx, esi
            mov         eax, dword [esi+0Ah]    ; start_block
            cmp         byte [.ver], 0
            jz          @f
            mov         eax, dword [esi+0Bh]    ; start_block
@@:         mov         esi, edx
            xor         edx, edx
            mov         ecx, dword [.bs]
            mul         ecx                     ; * blocksize
            add         esi, eax                ; base +
            ; found! esi=data, ecx=size
            mov         ecx, dword [ebx+01Ah]   ; file_length
            cmp         byte [.ver], 0
            jz          @f
            mov         ecx, dword [ebx+01Bh]   ; file_length
@@:         ret
.err:       xor         ecx, ecx
            ret
.ks:        dd          0
.bs:        dd          0
.ver:       dd          0

; ----------- JamesMolloy's ----------
; Find the kernel on initrd
; IN:   esi: initrd pointer, ecx: initrd end, edi: kernel filename
; OUT:  On Success
;         esi: pointer to the first byte, ecx: size in bytes
;       On Error
;         ecx: 0
jamesm_initrd:
            ; no real magic, so we assume initrd contains at least 2 files...
            cmp         word [esi+2], 0
            jne          .err
            cmp         byte [esi+4], 0BFh
            jne          .err
            cmp         byte [esi+77], 0BFh
            jne          .err
            ; upper bound
            xor         ecx, ecx
            ; strlen(kernel)
            mov         eax, edi
            or          eax, eax
            jz          .err
            cmp         byte [eax], 0
            jz          .err
            xor         ecx, ecx
@@:         inc         ecx
            inc         eax
            cmp         byte [eax], 0
            jnz         @b
            mov         dword [.ks], ecx
            mov         ebx, esi
            ; edx=*(int*)initrd_p
            lodsd
            mov         edx, eax
            ; for(i=0;i<nf && ptr[0]==0xBF;i++)
@@:         lodsb
            cmp         al, 0BFh
            jne         .err
            push        esi                 ; name equals?
            push        edi
            mov         ecx, dword [.ks]
            repz        cmpsb
            pop         edi
            pop         esi
            jz          @f
            add         esi, 72
            dec         dx
            jnz         @b
.err:       xor         ecx, ecx
            ret
@@:         mov         ecx, dword [esi+68]
            mov         esi, dword [esi+64]
            add         esi, ebx
            ret
.ks:        dd          0
