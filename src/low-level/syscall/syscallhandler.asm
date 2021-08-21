global syscallHandler

syscallHandler:
    push rcx
    push r11
    mov rcx, r10
    sti

    extern syscallHandlers
    call [rax * 8 + syscallHandlers]

    cli
    pop r11
    pop rcx
    o64 sysret
