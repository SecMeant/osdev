#include "types.h"
#include "textmode.h"
#include "memory.h"
#include "vm.h"

struct kernel_boot_header
{
	u64 kernel_base;
	u64 kernel_end;
	u64 pml4;

	// We use it to make ram_info array aligned to 16 bytes. This makes
	// some code slightly easier/shorter in bootloader when detecting
	// memory.
	u64 reserved; 

	struct ram_info_entry ram_info[64];
	u64 ram_info_entries;
};

int kmain(struct kernel_boot_header *boot_header)
{
	disable_cursor();

	txmbuf term = make_early_txmbuf();
	struct kernel_heap heap;

	txm_print(&term, "Kernel successfuly loaded from ELF!");
	txm_line_feed(&term);

	txm_print(&term, "Kernel relocated @ 0x");
	txm_print_hex(&term, boot_header->kernel_base);
	txm_line_feed(&term);

	txm_print(&term, "Kernel end @ 0x");
	txm_print_hex(&term, boot_header->kernel_end);
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
	txm_line_feed(&term);

	PML4E *pml4 = (PML4E*) boot_header->pml4;

	heap = make_early_heap();

	txm_print(&term, "Early heap @ ");
	txm_print_hex(&term, (u64) heap.begin);
	txm_putc(&term, '-');
	txm_print_hex(&term, (u64) heap.end);
	txm_line_feed(&term);

	PML4E *paging = kalloc(&heap, sizeof(PML4E) * 512, 4096);

	txm_print(&term, "Paging    = ");
	txm_print_hex(&term, (u64) paging);
	txm_line_feed(&term);

	memcpy(paging, (void*) boot_header->pml4, sizeof(PML4E) * 512);

	union {
		u64 as_u64;

		struct {
			u64 offset   : 12;
			u64 table    : 9;
			u64 dir      : 9;
			u64 dirptr   : 9;
			u64 pml4     : 9;
			u64 reserved : 16;
		};
	} va;
	_Static_assert(sizeof(va) == sizeof(u64));

	va.offset = 0;
	va.table = 0;
	va.dir = 0;
	va.dirptr = 1;
	va.pml4 = 0;

	int ret;
	ret = vmmap_4kb(&heap, pml4, (void*) va.as_u64, heap.begin + 0x6000);

	txm_print(&term, "VA        = ");
	txm_print_hex(&term, va.as_u64);
	txm_line_feed(&term);

	txm_print(&term, "ret       = ");
	txm_print_hex(&term, ret);
	txm_line_feed(&term);

	txm_print(&term, "heap.head = ");
	txm_print_hex(&term, (u64) heap.head);
	txm_line_feed(&term);

	txm_print(&term, "pml4[0]   = ");
	txm_print_hex(&term, (u64) pml4[0].as_u64);
	txm_line_feed(&term);

	PDPTE *pdpt = (PDPTE*) ((u64) pml4[0].address << 12);
	txm_print(&term, "pdpt[1]   = ");
	txm_print_hex(&term, (u64) pdpt[0].as_u64);
	txm_line_feed(&term);

	txm_print(&term, "pdpt[1]1g = ");
	txm_print_hex(&term, (u64) pdpt[1].is_1gb);
	txm_line_feed(&term);

	pdpt = pml4[0].address << 12;
	txm_print(&term, "pdpt[1]   = ");
	txm_print_hex(&term, (u64) pdpt[0].as_u64);
	txm_line_feed(&term);

	txm_print(&term, "pdpt[1]1g = ");
	txm_print_hex(&term, (u64) pdpt[1].is_1gb);
	txm_line_feed(&term);

	volatile u64 *ppp = va.as_u64;
	*ppp = 0x1337;

	while (1) {}
}
