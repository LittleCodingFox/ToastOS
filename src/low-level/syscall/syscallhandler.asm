global syscallHandler

syscallHandler:
    cli
    push rcx
    push r11
    mov rcx, r10

    extern syscallHandlers
    call [rax * 8 + syscallHandlers]

    pop r11
    pop rcx
    sti
    o64 sysret
