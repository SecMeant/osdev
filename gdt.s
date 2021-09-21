GLOBAL gdt_addr ,gdt ,gdt_end ,gdt64_addr ,gdt64 ,gdt64_end

SECTION .gdt32
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

SECTION .gdt64
gdt64_addr:
dw (gdt64_end - gdt64) - 1
dq gdt64

gdt64:
; null segment
dd 0, 0

dd 0xffff
dd (10 << 8)|(1 << 12)|(1<<15)|(0xf<<16)|(1<<21)|(1<<23)

dd 0xffff
dd (2 << 8)|(1 << 12)|(1<<15)|(0xf<<16)|(1<<21)|(1<<23)

gdt64_end:

