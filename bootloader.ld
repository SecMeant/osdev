OUTPUT_FORMAT("binary")
ENTRY(start)

INPUT
(
	stage1.o
	stage2.o
	gdt.o
)

MEMORY
{
	bootsec	(rwx) : ORIGIN = 0x7c00, LENGTH = 0x200
	initram	(rwx) : ORIGIN = 0x7e00, LENGTH = 0x20000
}

SECTIONS
{
	.stage1 : { KEEP(*(.stage1)) } > bootsec
	.stage2 : { KEEP(*(.stage2)) } > initram
	.gdt32 ALIGN(4096) : { KEEP(*(.gdt32)) } > initram
	.gdt64 ALIGN(4096) : { KEEP(*(.gdt64)) } > initram

	. = ALIGN(4096);
	kernel = .;
}
