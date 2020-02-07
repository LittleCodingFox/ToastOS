;*
;* x86_64-bios/tinf.inc
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
;* @brief Tiny inflate, ported after tinflate.c by bzt
;*

;IN:
; esi: gzipped initrd (without gzip header)
; edi: output buffer (guaranteed to be big enough)
; ecx: output buffer size
tinf_uncompress:
            push        ecx
            push        edi
            mov         edi, tinf_bss_start
            mov         ecx, tinf_bss_end-tinf_bss_start+1
            xor         al, al
            repnz       stosb
            pop         edi
            pop         ecx
            mov         dword [d_end], ecx
            add         dword [d_end], edi
.again:
            ; start a new block
            cmp         byte [d_btype], 255
            jne         .notst
            ; read final block flag
.next_blk:  call        tinf_getbit
            mov         byte [d_bfinal], al
            ; read block type
            xor         ebx, ebx
            mov         cl, 2
            call        tinf_read_bits
            mov         byte [d_btype], al
            ; build fixed huffman trees
            cmp         al, 1
            jne         @f
            call        tinf_build_fixed_trees
            xor         al, al
            ; decode trees from stream
@@:         cmp         al, 2
            jne         @f
            call        tinf_decode_trees
@@:
.notst:
            ; process current block
            cmp         byte [d_btype], 0
            jnz         @f
            ; decompress uncompressed block
            call        tinf_inflate_uncompressed_block
            JMP         .procend
@@:         cmp         byte [d_btype], 1
            je          @f
            cmp         byte [d_btype], 2
            jne         tinf_err
            ; decompress block with fixed/dyanamic huffman trees
            ; trees were decoded previously, so it's the same routine for both
@@:         call        tinf_inflate_block_data
.procend:   cmp         al, 1
            jne         @f
            cmp         byte [d_bfinal], 0
            ; the block has ended (without producing more data), but we
            ; can't return without data, so start procesing next block
            jz          .next_blk
            ret
@@:         or          al, al
            jnz         tinf_err
            cmp         edi, dword [d_end]
            jbe         .again
            ret

; build the fixed huffman trees
tinf_build_fixed_trees:
            push        edi
            xor         ecx, ecx
            xor         eax, eax
            ; build fixed length tree
            mov         cl, 7
            mov         edi, d_ltree_table
            repnz       stosb
            mov         al, 24
            stosb
            mov         al, 152
            stosb
            mov         al, 112
            stosb
            mov         edi, d_ltree_trans

            mov         cl, 24
            mov         ax, 256
@@:         stosw
            inc         ax
            dec         cl
            jnz         @b

            mov         cl, 144
            xor         ax, ax
@@:         stosw
            inc         ax
            dec         cl
            jnz         @b

            mov         cl, 8
            mov         ax, 280
@@:         stosw
            inc         ax
            dec         cl
            jnz         @b

            mov         cl, 112
            mov         ax, 144
@@:         stosw
            inc         ax
            dec         cl
            jnz         @b

            ; build fixed distance tree
            mov         cl, 5
            mov         edi, d_dtree_table
            xor         al, al
            repnz       stosb
            mov         al, 32
            stosb
            
            mov         edi, d_dtree_trans
            mov         cl, 32
            xor         ax, ax
@@:         stosw
            inc         ax
            dec         cl
            jnz         @b

            pop         edi
            ret

;IN:
; ebp: TINF_TREE
; ebx: lengths
; ecx: num
tinf_build_tree:
            push        edi
            xor         eax, eax
            ; clear code length count table
            mov         edi, ebp
            stosd                       ; for(i=0;i<16;i++) table[i]=0;
            stosd
            stosd
            stosd
            
            ; scan symbol lengths, and sum code length counts
            push        ebx
            push        ecx
@@:         mov         al, byte [ebx]  ;lengths[i]
            inc         ebx
            inc         byte [ebp+eax]  ;table[lengths[i]]++
            dec         cx
            jnz         @b
            mov         byte [ebp], 0

            ; compute offset table for distribution sort
            mov         cl, 16
            xor         edx, edx    ;i
            xor         ebx, ebx    ;sum
            xor         eax, eax
@@:         mov         word [offs+2*edx], bx
            mov         al, byte [ebp+edx]
            add         ebx, eax
            inc         edx
            dec         cl
            jnz         @b

            pop         ecx
            pop         ebx         ;lengths

            ; create code->symbol translation table (symbols sorted by code)
            xor         eax, eax    ;i
@@:         cmp         byte [ebx], 0
            jz          .null
            xor         edx, edx
            mov         dl, byte [ebx]          ;lengths[i]
            inc         word [offs+2*edx]
            mov         dx, word [offs+2*edx]
            dec         dx
            mov         word [ebp+2*edx+16], ax
.null:      inc         ebx
            inc         eax
            dec         cx
            jnz         @b

            pop         edi
            ret

tinf_decode_trees:
            mov         word [num], 0

            ; get 5 bits HLIT (257-286)
            xor         ecx, ecx
            mov         cl, 5
            mov         ebx, 257
            call        tinf_read_bits
            mov         word [hlit], ax
            mov         word [num], ax

            ; get 5 bits HDIST (1-32)
            mov         cl, 5
            xor         ebx, ebx
            inc         ebx
            call        tinf_read_bits
            mov         word [hdist], ax
            add         word [num], ax

            ; get 4 bits HCLEN (4-19)
            mov         cl, 4
            mov         ebx, ecx
            call        tinf_read_bits
            mov         word [hclen], ax

            push        edi
            mov         cl, 19
            mov         edi, lengths
            xor         ax, ax
            repnz       stosw
            pop         edi

            ; read code lengths for code length alphabet
            mov         edx, clcidx
            ; get 3 bits code length (0-7)
@@:         mov         cx, 3
            xor         ebx, ebx
            call        tinf_read_bits
            xor         ebx, ebx
            mov         bl, byte [edx]  ;clcidx[i]
            mov         byte[ebx+lengths], al
            inc         edx
            dec         word [hclen]
            jnz         @b

            ; build code length tree, temporarily use length tree
            mov         ebp, d_ltree
            mov         ebx, lengths
            xor         ecx, ecx
            mov         cl, 19
            call        tinf_build_tree
            
            ; decode code lengths for the dynamic trees
            mov         edx, lengths
.decode:    mov         ebp, d_ltree
            call        tinf_decode_symbol
            xor         ebx, ebx
            cmp         al, 16
            jne         .not16
            ; copy previous code length 3-6 times (read 2 bits)
            mov         cl, 2
            mov         bl, 3
            call        tinf_read_bits
            mov         cx, ax
            mov         al, byte [edx-1]    ;lengths[num-1]
@@:         mov         byte [edx], al
            inc         edx
            dec         word [num]
            dec         cl
            jnz         @b
            jmp         .next

.not16:     cmp         al, 17
            jne         .not17
            ; repeat code length 0 for 3-10 times (read 3 bits)
            mov         cl, 3
            mov         bl, cl
            call        tinf_read_bits
            mov         cx, ax
@@:         mov         byte [edx], 0
            inc         edx
            dec         word [num]
            dec         cl
            jnz         @b
            jmp         .next

.not17:     cmp         al, 18
            jne         .not18
            ; repeat code length 0 for 11-138 times (read 7 bits)
            mov         cl, 7
            mov         bl, 11
            call        tinf_read_bits
            mov         cx, ax
@@:         mov         byte [edx], 0
            inc         edx
            dec         word [num]
            dec         cl
            jnz         @b
            jmp         .next

.not18:     ; values 0-15 represent the actual code lengths
            mov         byte [edx], al
            inc         edx
            dec         word [num]
      
.next:      cmp         word [num], 0
            jnz         .decode

            ; build dynamic trees
            mov         ebp, d_ltree
            mov         ebx, lengths
            xor         ecx, ecx
            mov         cx, word [hlit]
            call        tinf_build_tree

            mov         ebp, d_dtree
            mov         ebx, lengths
            xor         ecx, ecx
            mov         cx, word [hlit]
            add         ebx, ecx
            mov         cx, word [hdist]
            call        tinf_build_tree
            ret

;OUT:
; al: status
tinf_inflate_block_data:
            cmp         word [d_curlen], 0
            jne         .notnull
            mov         ebp, d_ltree
            call        tinf_decode_symbol
            ; literal byte
            cmp         ax, 256
            jae         @f
            stosb
            xor         al, al
            ret
@@:         cmp         ax, 256
            jne         @f
            ; end of block
            mov         al, 1
            ret
@@:         ; substring from sliding dictionary
            sub         eax, 257
            ; possibly get more bits from length code
            xor         ecx, ecx
            mov         cl, byte [length_bits+eax]
            xor         ebx, ebx
            mov         bx, word [length_base+2*eax]
            call        tinf_read_bits
            mov         word [d_curlen], ax
            ; possibly get more bits from distance code
            mov         ebp, d_dtree
            call        tinf_decode_symbol
            xor         ecx, ecx
            mov         cl, byte [dist_bits+eax]
            xor         ebx, ebx
            mov         bx, word [dists_base+2*eax]
            call        tinf_read_bits
            cmp         dword [d_dict_ring], 0
            neg         eax
            mov         dword [d_lzOff], eax
.notnull:   mov         eax, edi
            add         eax, dword [d_lzOff]
            mov         al, byte [eax]
            stosb
@@:         dec         word [d_curlen]
            xor         al, al
            ret

;OUT:
; al: status
tinf_inflate_uncompressed_block:
            cmp         word [d_curlen], 0
            jne         @f
            ; get length
            lodsw
            ; get one's complement of length
            add         esi, 2
            ; increment length to properly return TINF_DONE below, without
            ; producing data at the same time
            mov         word [d_curlen], ax
            inc         word [d_curlen]
            ; make sure we start next block on a byte boundary
            mov         byte [d_bitcount], 0
@@:         dec         byte [d_curlen]
            cmp         byte [d_curlen], 0
            jnz         @f
            mov         al, 1
            ret
@@:         movsb
            xor         al, al
            ret

;OUT:
; al,zeroflag bit
tinf_getbit:
            mov         al, byte [d_bitcount]
            or          al, al
            jnz         @f
            lodsb
            mov         byte [d_tag], al
            mov         byte [d_bitcount], 8
@@:         dec         byte [d_bitcount]
            xor         ax, ax
            shr         byte [d_tag], 1
            adc         ax, 0
            ret

;IN:
; ebx: base
; cl: num
;OUT:
; eax: bits
tinf_read_bits:
            push        edx
            or          cl, cl
            jz          .end
            xor         eax, eax
            xor         edx, edx
            inc         dl
.next:      call        tinf_getbit
            jz          @f
            add         ebx, edx
@@:         shl         edx, 1
            dec         cl
            jnz         .next
.end:       pop         edx
            mov         eax, ebx
            ret

;IN:
; ebp: TINF_TREE
;OUT:
; eax: trans
tinf_decode_symbol:
            push        edx
            xor         eax, eax
            xor         ebx, ebx ;cur
            xor         ecx, ecx ;len
            xor         edx, edx ;sum
            ; get more bits while code value is above sum
@@:         shl         ebx, 1
            call        tinf_getbit
            add         ebx, eax
            inc         ecx
            mov         al, byte [ebp+ecx]
            add         edx, eax
            sub         ebx, eax
            jns         @b
            add         edx, ebx
            mov         ax, word [ebp+2*edx+16]
mov ebp, edx
            pop         edx
            ret

tinf_err:
            mov         esi, nogzip
            jmp         prot_diefunc

length_bits:
            db          0, 0, 0, 0, 0, 0, 0, 0
            db          1, 1, 1, 1, 2, 2, 2, 2
            db          3, 3, 3, 3, 4, 4, 4, 4
            db          5, 5, 5, 5, 0, 0, 0, 0
length_base:
            dw          3, 4, 5, 6, 7, 8, 9, 10
            dw          11, 13, 15, 17, 19, 23, 27, 31
            dw          35, 43, 51, 59, 67, 83, 99, 115
            dw          131, 163, 195, 227, 258, 0, 0
dist_bits:
            db          0, 0, 0, 0, 1, 1, 2, 2
            db          3, 3, 4, 4, 5, 5, 6, 6
            db          7, 7, 8, 8, 9, 9, 10, 10
            db          11, 11, 12, 12, 13, 13, 0, 0
dists_base:
            dw          1, 2, 3, 4, 5, 7, 9, 13
            dw          17, 25, 33, 49, 65, 97, 129, 193
            dw          257, 385, 513, 769, 1025, 1537, 2049, 3073
            dw          4097, 6145, 8193, 12289, 16385, 24577, 0, 0
clcidx:
            db          16, 17, 18, 0, 8, 7, 9, 6
            db          10, 5, 11, 4, 12, 3, 13, 2
            db          14, 1, 15

d_btype:    db          255
