global SwitchTasks

SwitchTasks:

	push qword [rdi + 0]	; ss
	push qword [rdi + 8]	; rsp
	push qword [rdi + 16]	; rflags
	push qword [rdi + 24]	; cs
	push qword [rdi + 32]	; rip

	; load registers from new task
	mov rbx, [rdi + 48]
	mov rcx, [rdi + 56] 
	mov rdx, [rdi + 64] 
	mov rsi, [rdi + 72] 
	mov rbp, [rdi + 80]
	mov r8,  [rdi + 88]
	mov r9,  [rdi + 96]
	mov r10, [rdi + 104] 
	mov r11, [rdi + 112] 
	mov r12, [rdi + 120] 
	mov r13, [rdi + 128] 
	mov r14, [rdi + 136]
	mov r15, [rdi + 144]

	; update cr3
	mov rax, [rdi + 160] 	; copy to rax
	mov cr3, rax			; set cr3
	mov rax, [rdi + 40] 	; set proper rax value

	; will be overwritten, so do it last
	mov rdi, [rdi + 152]

	iretq
