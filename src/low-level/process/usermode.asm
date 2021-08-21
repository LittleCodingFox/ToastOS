global SwitchToUsermode

SwitchToUsermode:
    mov rcx, rdi
    mov rsp, rsi
    mov r11, 0x0202
    o64 sysret