/*
* Copyright (c) 2017 bzt (bztsrc@gitlab)
* https://creativecommons.org/licenses/by-nc-sa/4.0/
*
* void *memcpy(void *dst, const void *src, size_t len)
*/
.global sse_memcpy
sse_memcpy:
    cld
    /* check input parameters */
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
    orq     %rdx, %rdx
    jz      2f
    /* if it's a small block */
    cmpq    $512, %rdx
    jb      1f
    /* if both source and destination aligned */
    movb    %sil, %al
    xorb    %dil, %al
    andb    $15, %al
    jnz     1f
    /* copy big blocks, 256 bytes per iteration */
0:  movq    %rdx, %rcx
    xorq    %rdx, %rdx
    movb    %cl, %dl
    shrq    $8, %rcx
0:  prefetchnta 256(%rsi)
    prefetchnta 288(%rsi)
    prefetchnta 320(%rsi)
    prefetchnta 352(%rsi)
    prefetchnta 384(%rsi)
    prefetchnta 416(%rsi)
    prefetchnta 448(%rsi)
    prefetchnta 480(%rsi)
    movdqa    0(%rsi), %xmm0
    movdqa   16(%rsi), %xmm1
    movdqa   32(%rsi), %xmm2
    movdqa   48(%rsi), %xmm3
    movdqa   64(%rsi), %xmm4
    movdqa   80(%rsi), %xmm5
    movdqa   96(%rsi), %xmm6
    movdqa  112(%rsi), %xmm7
    movdqa  128(%rsi), %xmm8
    movdqa  144(%rsi), %xmm9
    movdqa  160(%rsi), %xmm10
    movdqa  176(%rsi), %xmm11
    movdqa  192(%rsi), %xmm12
    movdqa  208(%rsi), %xmm13
    movdqa  224(%rsi), %xmm14
    movdqa  240(%rsi), %xmm15
    movntdq %xmm0,   0(%rdi)
    movntdq %xmm1,  16(%rdi)
    movntdq %xmm2,  32(%rdi)
    movntdq %xmm3,  48(%rdi)
    movntdq %xmm4,  64(%rdi)
    movntdq %xmm5,  80(%rdi)
    movntdq %xmm6,  96(%rdi)
    movntdq %xmm7, 112(%rdi)
    movntdq %xmm8, 128(%rdi)
    movntdq %xmm9, 144(%rdi)
    movntdq %xmm10,160(%rdi)
    movntdq %xmm11,176(%rdi)
    movntdq %xmm12,192(%rdi)
    movntdq %xmm13,208(%rdi)
    movntdq %xmm14,224(%rdi)
    movntdq %xmm15,240(%rdi)
    addq    $256, %rsi
    addq    $256, %rdi
    decq    %rcx
    jnz     0b
    /* copy small block */
1:  movq    %rdx, %rcx
    shrq    $3, %rcx
    or      %rcx, %rcx
    jz      1f
    repnz   movsq
1:  movb    %dl, %cl
    andb    $0x7, %cl
    jz      2f
    repnz   movsb
2:  movq    %rdi, %rax
    ret