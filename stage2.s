[bits 16]

[org 0x7e00]
stage2:
	;push stage2_win
	;call puts16
	;add sp, 2

	cli
	lgdt [gdt_addr]

	mov eax, cr0
	or al, 1
	mov cr0, eax

	;sti

	jmp 0x8:start32

putc16:
	push bp
	mov bp, sp

	push ax
	push bx
	push cx

	mov ah, 0x0e
	mov bh, 0
	mov bl, 4
	mov cx, 5

	mov al, [bp+4]
	int 0x10

	pop cx
	pop bx
	pop ax

	mov sp, bp
	pop bp
	ret

puts16:
	push bp
	mov bp, sp

	.print_loop:
	mov si, [bp+4]
	xor al, al
	mov al, [si]
	add si, 1
	mov [bp+4], si
	cmp al, 0
	jz .str_end
	push ax
	call putc16
	add sp, 2
	jmp .print_loop

	.str_end:
	mov sp, bp
	pop bp
	ret

gdt_addr:
dw gdt_end - gdt
dd gdt

; gdt alignment
times (32 - ($ - $$) % 32) db 0xcc
gdt:

; null segment
dd 0, 0

dd 0xffff
dd (10 << 8)|(1 << 12)|(1<<15)|(0xf<<16)|(1<<22)|(1<<23)

dd 0xffff
dd (2 << 8)|(1 << 12)|(1<<15)|(0xf<<16)|(1<<22)|(1<<23)

gdt_end:

times (32 - ($ - $$) % 32) db 0xcc
[bits 32]

%define STAGE_POS (0xb8000 + 0x140)

start32:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov ss, ax

	mov ebp, 0x9000
	mov esp, 0x9000

	push 0
	call set_stage
	add esp, 4

	call print_stage

	; Enable A20 line (doesnt work on very old machines I dont care)
	in al, 0x92
	or al, 2
	out 0x92, al

	call check_a20
	test eax, eax
	je .longmode_setup_failed

	push 1
	call set_stage
	add esp, 4
	call print_stage

	call check_cpuid_available
	test eax, eax
	je .longmode_setup_failed

	push 2
	call set_stage
	add esp, 4
	call print_stage

	call check_longmode_available
	test eax, eax
	je .longmode_setup_failed

	push 3
	call set_stage
	add esp, 4
	call print_stage

	.longmode_setup_failed:
	jmp $

check_a20:
	pushad
	mov edi, 0x112345
	mov esi, 0x012345
	mov [edi], edi
	mov [esi], esi
	cmpsd
	popad
	mov eax, 1
	je .a20_disabled
	ret
	.a20_disabled:
		xor eax, eax
		ret

; void print_stage(void)
print_stage:
	push ebp
	mov ebp, esp

	lea eax, [stage]
	mov eax, [eax]
	and eax, 0xff      ; leave only ascii of stage (clear color bits)
	or eax, 0x00000200 ; black bg, green font

	lea edx, [STAGE_POS]
	mov dword [edx], eax

	mov esp, ebp
	pop ebp
	ret

; void set_stage(u32 stage);
set_stage:
	push ebp
	mov ebp, esp

	mov eax, [ebp+8]
	add eax, 0x30
	lea edx, [stage]
	mov [edx], eax

	mov esp, ebp
	pop ebp
	ret

check_cpuid_available:
	pushfd
	pop eax

	; save old flags to ecx
	mov ecx, eax

	; flip ID bit
	xor eax, 1 << 21

	push eax
	popfd

	pushfd
	pop eax

	; restore old flags
	push ecx
	popfd

	cmp eax, ecx
	mov eax, 0
	setne al
	ret

check_longmode_available:
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000001
	mov eax, 0
	setge al
	ret

stage2_win: db 'Second stage loaded!', 0xd, 0xa, 0
msg_a20_enabled: db 'A20 line is enabled', 0xd, 0xa, 0
msg_a20_disabled: db 'A20 line is disabled', 0xd, 0xa, 0
stage: dd 0 ; filled at runtime

