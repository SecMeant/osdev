#include "types.h"
#include "kernel.h"
#include "textmode.h"
#include "vm.h"
#include "irq.h"
#include "std.h"
#include "abort.h"
#include "acpi.h"

struct kernel_boot_header boot_header;

static int check_has_apic(void)
{
	int ret = 0;

	__asm__ volatile (
		"mov $1, %%eax\n"
		"cpuid\n"
		: "=d" (ret) : : "rax", "ebx", "ecx"
	);

	return ret & (1ull << 9);
}

void kmain(struct kernel_boot_header *bios_boot_header)
{
	memcpy(&boot_header, bios_boot_header, sizeof(struct kernel_boot_header));

	disable_cursor();

	earlytxm = make_early_txmbuf();
	struct kernel_heap heap;

	txm_clear_screen(&earlytxm);

	txm_print(&earlytxm, "Kernel successfuly loaded from ELF!");
	txm_line_feed(&earlytxm);

	txm_print(&earlytxm, "Kernel relocated @ 0x");
	txm_print_hex(&earlytxm, boot_header.kernel_base);
	txm_line_feed(&earlytxm);

	txm_print(&earlytxm, "Kernel end       @ 0x");
	txm_print_hex(&earlytxm, boot_header.kernel_end);
	txm_line_feed(&earlytxm);

	txm_print(&earlytxm, "PML4 allocated   @ 0x");
	txm_print_hex(&earlytxm, boot_header.pml4);
	txm_line_feed(&earlytxm);

	txm_line_feed(&earlytxm);
	txm_print(&earlytxm, "Detected RAM:");
	txm_line_feed(&earlytxm);
	txm_print(&earlytxm, "|       BASE       |       SIZE       |       TYPE       |");
	txm_line_feed(&earlytxm);
	txm_line_feed(&earlytxm);

	for (u64 i = 0; i < boot_header.ram_info_entries; ++i) {
		const struct ram_info_entry e = boot_header.ram_info[i];
		txm_putc(&earlytxm, '|');
		txm_putc(&earlytxm, ' ');
		txm_print_hex(&earlytxm, e.base);
		txm_putc(&earlytxm, ' ');
		txm_putc(&earlytxm, '|');
		txm_putc(&earlytxm, ' ');
		txm_print_hex(&earlytxm, e.size);
		txm_print(&earlytxm, e.type == RAM_INFO_TYPE_FREE ?
			" |       FREE       |" : " |     RESERVED     |");
		txm_line_feed(&earlytxm);
	}
	txm_line_feed(&earlytxm);

	PML4E *pml4 = (PML4E*) boot_header.pml4;

	heap = make_early_heap();

	txm_print(&earlytxm, "Early heap @ ");
	txm_print_hex(&earlytxm, (u64) heap.begin);
	txm_putc(&earlytxm, '-');
	txm_print_hex(&earlytxm, (u64) heap.end);
	txm_line_feed(&earlytxm);

	PML4E *paging = kalloc(&heap, sizeof(PML4E) * 512, 4096);

	txm_print(&earlytxm, "Paging    = ");
	txm_print_hex(&earlytxm, (u64) paging);
	txm_line_feed(&earlytxm);

	memcpy(paging, (void*) boot_header.pml4, sizeof(PML4E) * 512);

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
	va.table = 1;
	va.dir = 0;
	va.dirptr = 1;
	va.pml4 = 0;

	const int ret = vmmap_4kb(&heap, pml4, (void*) va.as_u64, heap.begin);

	txm_print(&earlytxm, "VA        = ");
	txm_print_hex(&earlytxm, va.as_u64);
	txm_line_feed(&earlytxm);

	if (ret) {
		txm_print(&earlytxm, "mapping VA failed");
		txm_print_hex(&earlytxm, ret);
		txm_line_feed(&earlytxm);
	}

	txm_print(&earlytxm, "heap.head = ");
	txm_print_hex(&earlytxm, (u64) heap.head);
	txm_line_feed(&earlytxm);

	txm_print(&earlytxm, "pml4[0]   = ");
	txm_print_hex(&earlytxm, (u64) pml4[0].as_u64);
	txm_line_feed(&earlytxm);

	PDPTE *pdpt = (PDPTE*) ((u64) pml4[0].address << 12);
	txm_print(&earlytxm, "pdpt[0]   = ");
	txm_print_hex(&earlytxm, (u64) pdpt[0].as_u64);
	txm_line_feed(&earlytxm);

	txm_print(&earlytxm, "pdpt[0]1g = ");
	txm_print_hex(&earlytxm, (u64) pdpt[0].is_1gb);
	txm_line_feed(&earlytxm);

	txm_print(&earlytxm, "pdpt[1]   = ");
	txm_print_hex(&earlytxm, (u64) pdpt[1].as_u64);
	txm_line_feed(&earlytxm);

	txm_print(&earlytxm, "pdpt[1]1g = ");
	txm_print_hex(&earlytxm, (u64) pdpt[1].is_1gb);
	txm_line_feed(&earlytxm);

	load_idt();

	txm_print(&earlytxm, "IRQ #20h  @ ");
	txm_print_hex(&earlytxm, idtgd_get_64bit_offset(idt[0x20]));
	txm_line_feed(&earlytxm);

	const acpi_table_rsdp *rsdp = acpi_scan_bios_for_rsdp();
	txm_print(&earlytxm, "RSDP      @ ");
	txm_print_hex(&earlytxm, (u64)rsdp);
	txm_line_feed(&earlytxm);

	if (!rsdp) {
		txm_line_feed(&earlytxm);
		txm_print(&earlytxm, "Could not find RSDP");
		panic();
	}

	const int has_apic = check_has_apic();
	if (!has_apic) {
		txm_line_feed(&earlytxm);
		txm_print(&earlytxm, "APIC not supported");
		panic();
	}

	setup_pic();

	__asm__ volatile ("sti\n");

	for(;;)
		__asm__ volatile ("hlt\n");
}

