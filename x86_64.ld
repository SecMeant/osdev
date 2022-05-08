ENTRY(kmain)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS {
    . = ALIGN(4096);
    .text : {
        __text_start = .;
        *(.text*)
        __text_end = .;
    }

    . = ALIGN(4096);
    .rodata : {
        __rodata_start = .;
        *(.rodata*)
        __rodata_end = .;
    }

    . = ALIGN(4096);
    .data : {
        __data_start = .;
        *(.data*)
        __data_end = .;
    }

    . = ALIGN(4096);
    .bss : {
        __bss_start = .;
        *(.bss*)
        __bss_end = .;
    }

    __end = .;

    /DISCARD/ : {
        *(.comment*)
        *(.eh_frame*)
        *(.gcc_except_table*)
        *(.note*)
        *(.rel.eh_frame*)
    }
}