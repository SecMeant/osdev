#include "types.h"
#include "textmode.h"
#include "memory.h"
#include "vm.h"

struct kernel_boot_header
{
	u64 kbase;
	u64 pml4;
	struct ram_info_entry ram_info[64];
	u64 ram_info_entries;
};

static void memcpy(void *dst_, const void *src_, u64 size)
{
	u8 *dst = dst_;
	const u8 *src = src_;

	while (size) {
		*dst = *src;

		++dst;
		++src;
		--size;
	}
}

int kmain(struct kernel_boot_header *boot_header)
{
	txmbuf term = make_early_txmbuf();
	struct kernel_heap heap;

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
	txm_line_feed(&term);

	PML4E *pml4 = (PML4E*) boot_header->pml4;
	PDPTE *pdpte = (PDPTE*) (pml4[0].address << 12);

	heap = make_early_heap(pml4, boot_header->ram_info, boot_header->ram_info_entries);

	txm_print(&term, "Early heap @ ");
	txm_print_hex(&term, (u64) heap.begin);
	txm_putc(&term, '-');
	txm_print_hex(&term, (u64) heap.end);
	txm_line_feed(&term);

	PML4E *paging = kalloc(&heap, sizeof(PML4E) * 512, 4096);

	txm_print_hex(&term, (u64) paging);
	txm_line_feed(&term);
	txm_print_hex(&term, (u64) boot_header->pml4);
	txm_line_feed(&term);
	txm_print_hex(&term, (u64) pml4);
	txm_line_feed(&term);
	txm_print_hex(&term, (u64) pdpte);
	txm_line_feed(&term);
	txm_print_hex(&term, pdpte->raw);
	txm_line_feed(&term);
	txm_print_hex(&term, pdpte->pdpte_1gb.address << 12);
	txm_line_feed(&term);
	txm_print_hex(&term, pdpte->is_1gb);
	txm_line_feed(&term);

	memcpy(paging, (void*) boot_header->pml4, sizeof(PML4E) * 512);

	while (1) {}
}
