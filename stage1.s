[bits 16]
[org 0x7c00]

_start:
jmp word 0000:start
start:

push msg
call puts
add sp, 2

jmp $

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

msg: db 'Loading second stage of bootloader... ', 0xd, 0xa, 0

times 510-($-$$) db 0
db 0x55
db 0xaa
