[bits 16]
[org 0x7c00]

_start:
jmp word 0000:start
start:

mov ah, 0x0e
mov bh, 0
mov bl, 4
mov cx, 5

mov al, [msg]
int 0x10
jmp start

msg: db 'HELLO', 0

times 510-($-$$) db 0
db 0x55
db 0xaa
