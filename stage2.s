[bits 16]

[org 0x7e00]
stage2:
	;push stage2_win
	;call puts
	;add sp, 2

	cli
	lea ax, [gdt_addr]
	lgdt [gdt_addr]

	mov eax, cr0
	or al, 1
	mov cr0, eax

	;sti

	jmp 0x8:start32

putc:
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

puts:
	push bp
	mov bp, sp

	puts_print_loop:
	mov si, [bp+4]
	xor al, al
	mov al, [si]
	add si, 1
	mov [bp+4], si
	cmp al, 0
	jz puts_str_end
	push ax
	call putc
	add sp, 2
	jmp puts_print_loop

	puts_str_end:
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

stage2_win: db 'Second stage loaded!', 0xd, 0xa, 0

times (32 - ($ - $$) % 32) db 0xcc
[bits 32]
start32:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov ss, ax

	lea eax, [0xb8000]
	mov dword [eax], 0x41414141

	jmp $


