.intel_syntax noprefix

.section .text

.extern isr_default

.global irq_handler
irq_handler:
	push rax
	push rcx
	push rdx
	push r8
	push r9
	push r10
	push r11

	lea rdi, [rsp]
	call isr_default

	pop r11
	pop r10
	pop r9
	pop r8
	pop rdx
	pop rcx
	pop rax

	iretq

.global trap_handler
trap_handler:
	cli
	hlt
	iretq

