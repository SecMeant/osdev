.intel_syntax noprefix

.section .text

.extern isr_default
.extern isr_keyboard

.global irq_handler_keyboard
irq_handler_keyboard:
       push rdi
       push rax
       push rcx
       push rdx
       push r8
       push r9
       push r10
       push r11

       lea rdi, [rsp]
       call isr_keyboard

       pop r11
       pop r10
       pop r9
       pop r8
       pop rdx
       pop rcx
       pop rax
       pop rdi

       iretq

.global irq_handler
irq_handler:
	push rdi
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
	pop rdi

	iretq

.global trap_handler
trap_handler:
	cli
	hlt
	iretq

