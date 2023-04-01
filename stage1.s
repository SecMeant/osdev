[bits 16]

%include "boot_info.inc"

[org 0x7c00]
jmp word 0x0000:start

start:
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	mov ss, ax
	mov sp, 0x7c00

	mov ah, 0 ; set video mode
	mov al, 2 ; 80x25 8x8 640x200
	int 0x10

	jmp 0:reset_cs
reset_cs:

	mov bx, 0
	mov ds, bx
	mov ss, bx

	mov sp, 0x9000

	push msg
	call puts
	add sp, 2

	call load_stage2

stage2_fail:
	; we should never reach here
	push stage2_fail_msg
	call puts
	add sp, 2

	jmp $

load_stage2:
	mov ax, STAGE2_LOAD_SEG
	mov es, ax
	mov bx, STAGE2_LOAD_ADDR

	; TODO: Add support for LOADER_SEC_SIZE + KERNEL64_SEC_SIZE > 128.
	;       Now, this bios call will fail for big kernels ;/
	mov ah, 2      ; read sectors into memory
	mov al, LOADER_SEC_SIZE + KERNEL64_SEC_SIZE
	mov ch, 0
	mov cl, 2      ; read from 2nd sector (1st contains this code)
	mov dh, 0

	int 0x13

	jc stage2_fail

	; jump to second stage
	jmp stage2

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

msg: db 'Loading second stage of bootloader... ', 0xd, 0xa, 0
stage2_fail_msg: db 'Loading second stage of bootloader FAILED!', \
                     0xd, 0xa, 0

times 510-($-$$) db 0
db 0x55
db 0xaa

stage2:
