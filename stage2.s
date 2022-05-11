[bits 16]

%define KERNEL64_BASE 0x100000
%define KERNEL64_HEADER_ES 0x7f90
%define KERNEL64_HEADER_DI 0x0000
%define KERNEL64_HEADER ((KERNEL64_HEADER_ES << 4) | KERNEL64_HEADER_DI)
%define RAM_INFO_MAX_ENTRIES 64
%define SMAP 0x534d4150

[org 0x7e00]
stage2:
	push stage2_win
	call puts16
	add sp, 2

	mov ax, KERNEL64_HEADER_ES + 1
	mov es, ax
	mov di, KERNEL64_HEADER_DI

	call detect_mem_e820
	mov ax, 24 * RAM_INFO_MAX_ENTRIES
	xchg ax, di
	xor dx, dx
	mov bx, 24
	div bx
	mov [es:di], ax

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

; This function detects memory using INT 0x15, EAX=0xE820 filling info_array
; and returning how many entries for the array are valid.
;
; void detect_mem_e820(
;	struct ram_info_entry *info_array, // ES:DI
;	u64 size,
;	u64 *valid_entries
;)
detect_mem_e820:
	xor ebx, ebx
	mov edx, SMAP
	mov eax, 0xe820
	mov ecx, 24

.detect_mem_e820_loop:
	int 0x15

	jc .detect_mem_e820_failed

	cmp eax, SMAP
	jne .detect_mem_e820_failed

	test ebx, ebx
	jz .detect_mem_e820_loop_end

	add di, 24
	mov eax, 0xe820
	mov ecx, 24
	jmp .detect_mem_e820_loop

.detect_mem_e820_loop_end:
	ret

.detect_mem_e820_failed:
	jmp $

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

	mov eax, cr3
	or eax, PML4 - $$ + 0x7e00
	mov cr3, eax

	push 4
	call set_stage
	add esp, 4
	call print_stage

	mov eax, cr4
	or eax, 1 << 5 ; Enable PAE
	mov cr4, eax

	push 5
	call set_stage
	add esp, 4
	call print_stage

	mov ecx, 0xc0000080 ; read EFER
	rdmsr
	or eax, 1 << 8 ; Enable IA-32e mode
	wrmsr

	push 6
	call set_stage
	add esp, 4
	call print_stage

	mov eax, cr0
	or eax, 1 << 31 | 1 << 0 ; Enable paging
	mov cr0, eax

	push 7
	call set_stage
	add esp, 4
	call print_stage

	lgdt [gdt64_addr]
	jmp dword 0x8:start64

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

; Align PML4 to 4kB
times (4096 - ($ - $$ + 0x7e00) % 4096) db 0xcc
PML4:
dq 1 | 1 << 1 | (PDPTE - $$ + 0x7e00)
times 511 dq 0

PDPTE:
dq 1 | 1 << 1 | 1 << 7 | 0 << 30
times 511 dq 0

start64:
[bits 64]
	add rsp, 7
	and rsp, 0xfffffffffffffff8
	mov rbp, rsp

	;xor rax, rax
	;.debug:
	;test rax, rax
	;jz .debug

	; e_shoff
	mov r9, [kernel + 0x28]

	; e_shentsize
	mov ax, word [kernel + 0x3a]
	mov r10, rax

	; e_shnum
	mov ax, word [kernel + 0x3c]
	mov r11, rax

	; allocate space for section header
	; and for variables
	sub rsp, 0x40 + 0x18

	mov [rbp - 0x48], r9  ; offset to current section header
	mov [rbp - 0x50], r10 ; section size (const)
	mov [rbp - 0x58], r11 ; section count (how much more to read)

	;
	; map kernel
	;

	lea rdi, [rbp - 0x40]
	lea rsi, [r9 + kernel]
	mov rdx, r10
	call memcpy64

	lea rdi, [rbp - 0x40]
	mov rsi, r10
	call memcheckzeroed
	test rax, rax
	jnz .mapkernelfailed

	sub qword [rbp - 0x58], 1
	jz .mapkernelend

	.section_loop:
	mov rax, [rbp - 0x50]
	add [rbp - 0x48], rax
	mov r9,  [rbp - 0x48]

	; read section header
	lea rdi, [rbp - 0x40]
	lea rsi, [r9 + kernel]
	mov rdx, [rbp - 0x50]
	call memcpy64

	; copy section into memory
	lea rdi, [rbp - 0x40]
	mov rsi, kernel
	mov rdx, KERNEL64_BASE
	call mapsection

	sub qword [rbp - 0x58], 1
	jnz .section_loop

	.mapkernelend:

	mov rdi, KERNEL64_HEADER
	mov qword [rdi], KERNEL64_BASE
	mov qword [rdi + 8], PML4

	; e_entry
	mov rax, [kernel + 0x18]
	add rax, KERNEL64_BASE
	call rax

	.mapkernelfailed:
	jmp $

; void memcpy(rdi: char *dest, rsi: char *source, rdx: u64 size)
memcpy64:
	test rdx, rdx
	jz .exit

	.copy_loop:
	mov byte al, [rsi]
	mov byte [rdi], al

	inc rsi
	inc rdi
	dec rdx
	jnz .copy_loop

	.exit:
	ret

; u64 memcpy(rdi: char *m1, rsi: char *m2, rdx: u64 size)
; ret == 0 if mem is equal
; ret != 0 if mem is differs
memcmp64:
	xor rax, rax
	test rdx, rdx
	jz .exit

	.cmp_loop:
	mov byte al, [rdi]
	mov byte cl, [rsi]
	sub al, cl
	jnz .exit

	inc rsi
	inc rdi
	dec rdx
	jnz .cmp_loop

	.exit:
	ret

; u64 memcheckzeroed(rdi: char *m, rsi: u64 size)
; ret == 0 memory is zeroed
; ret != 0 memory is not zeroed
memcheckzeroed:
	xor rax, rax
	test rsi, rsi
	jz .exit

	.cmp_loop:
	mov byte al, [rdi]
	test al, al
	jnz .exit

	inc rdi
	dec rsi
	jnz .cmp_loop

	.exit:
	ret

; void mapsection(rdi: void *section_header, rsi: void *elf, rdx: void *base)
mapsection:
	mov r8,  [rdi + 0x10] ; sh_addr
	mov r9,  [rdi + 0x18] ; sh_offset
	mov rcx, [rdi + 0x20] ; sh_size
	mov dword eax, [rdi + 0x04] ; sh_type

	add r8, rdx ; actual (virtual) address of the section
	add r9, rsi ; address inside ELF to the section

	mov rdi, r8
	mov rsi, r9
	mov rdx, rcx

	cmp eax, 0x8 ; SHT_NOBITS (bss)
	je .mapbss

	cmp eax, 0x01 ; SHT_PROGBITS
	jne .exit

	call memcpy64
	jmp .exit

	.exit:
	ret

	.mapbss:
	xor eax, eax
	rep stosb
	jmp .exit

stage2_win: db 'Second stage loaded!', 0xd, 0xa, 0
msg_a20_enabled: db 'A20 line is enabled', 0xd, 0xa, 0
msg_a20_disabled: db 'A20 line is disabled', 0xd, 0xa, 0
stage: dd 0 ; filled at runtime

; Align kernel to 4kB
times (4096 - ($ - $$ + 0x7e00) % 4096) db 0xcc
kernel:
; Build script will append 64bit elf of the kernel just after stage2

