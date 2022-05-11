#include "types.h"
#include "textmode.h"

enum ram_info_type_e
{
	RAM_INFO_TYPE_FREE     = 1,
	RAM_INFO_TYPE_RESERVED = 2,
};

struct ram_info_entry
{
	u64 base;
	u64 size;
	u32 type;
	u32 attr; // ACPI 3.0 extended attribute bitfield
};

_Static_assert(
	sizeof(struct ram_info_entry) == 24,
	"Unexpected ram_info_entry size"
);

struct kernel_boot_header
{
	u64 kbase;
	u64 pml4;
	struct ram_info_entry ram_info[64];
	u64 ram_info_entries;
};

int kmain(struct kernel_boot_header *boot_header)
{
	txmbuf term = make_early_txmbuf();

	txm_print(&term, "Kernel successfuly loaded from ELF!");
	txm_line_feed(&term);

	txm_print(&term, "Kernel relocated @ 0x");
	txm_print_hex(&term, boot_header->kbase);
	txm_line_feed(&term);

	txm_print(&term, "PML4 allocated   @ 0x");
	txm_print_hex(&term, boot_header->pml4);
	txm_line_feed(&term);

	txm_line_feed(&term);
	txm_print(&term, "Detected RAM:");
	txm_line_feed(&term);
	txm_print(&term, "|       BASE       |       SIZE       |       TYPE       |");
	txm_line_feed(&term);
	txm_line_feed(&term);

	for (u64 i = 0; i < boot_header->ram_info_entries; ++i) {
		const struct ram_info_entry e = boot_header->ram_info[i];
		txm_putc(&term, '|');
		txm_putc(&term, ' ');
		txm_print_hex(&term, e.base);
		txm_putc(&term, ' ');
		txm_putc(&term, '|');
		txm_putc(&term, ' ');
		txm_print_hex(&term, e.size);
		txm_print(&term, e.type == RAM_INFO_TYPE_FREE ?
			" |       FREE       |" : " |     RESERVED     |");
		txm_line_feed(&term);
	}

	while (1) {}
}
