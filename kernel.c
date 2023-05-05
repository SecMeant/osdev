#include "types.h"
#include "kernel.h"
#include "textmode.h"
#include "vm.h"
#include "irq.h"
#include "std.h"
#include "abort.h"
#include "acpi.h"
#include "compiler.h"
#include "msr.h"
#include "apic.h"
#include "io.h"

#define KERNEL_BASE_VA 0xc0000000UL
#define APIC_BASE_SIZE 0x1000UL

// Map APIC registers just before the kernel.
#define APIC_BASE_VA (KERNEL_BASE_VA - APIC_BASE_SIZE)

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
	txm_print_hex(&earlytxm, boot_header.kernel_phys_base);
	txm_line_feed(&earlytxm);

	txm_print(&earlytxm, "Kernel end       @ 0x");
	txm_print_hex(&earlytxm, boot_header.kernel_phys_end);
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

	PML4E * volatile paging = kalloc(&heap, sizeof(PML4E) * 512, 4096);

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
	va.reserved = 0;

	const int ret = vmmap_4kb(&heap, pml4, (void*) va.as_u64, heap.begin);

	txm_print(&earlytxm, "VA        = ");
	txm_print_hex(&earlytxm, va.as_u64);
	txm_line_feed(&earlytxm);

	if (ret) {
		txm_print(&earlytxm, "mapping VA failed ");
		txm_putc(&earlytxm, '0');
		txm_print_hex(&earlytxm, ret);
		txm_line_feed(&earlytxm);
	}

	/* Remap kernel to VM and jump to it. */
	for (u64 p = boot_header.kernel_phys_base, v = KERNEL_BASE_VA; p < boot_header.kernel_phys_end; p += 4096, v += 4096) {
		if (vmmap_4kb(&heap, pml4, (void*) v, (void*) p)) {
			txm_print(&earlytxm, "mapping VA failed ");
			txm_putc(&earlytxm, '2');
			txm_line_feed(&earlytxm);
			txm_print_hex(&earlytxm, p);
			txm_line_feed(&earlytxm);
			txm_print_hex(&earlytxm, v);
			txm_line_feed(&earlytxm);

			panic();
		}
	}

	__asm__ volatile (
		"xor %%eax, %%eax\n"
		"call .L1\n"
		".L1:\n"
		"pop %%rax\n"
		"cmp %[kernel_virt_base], %%eax\n"
		"jnc .L2\n"
		"sub %[kernel_phys_base], %%rax\n"
		"add %[kernel_virt_base] + (.L2 - .L1), %%eax\n"
		"jmp *%%rax\n"
		".L2:\n"
		:
		: [kernel_phys_base] "r" (boot_header.kernel_phys_base)
		, [kernel_virt_base] "i" (KERNEL_BASE_VA)
		: "rax", "rbx", "memory"
	);

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

	const acpi_table_rsdp * const rsdp = acpi_scan_bios_for_rsdp();
	const acpi_table_rsdt * rsdt = 0;
	if (rsdp) {
		char oem_id[sizeof(rsdp->oem_id) + 1];
		memcpy(oem_id, rsdp->oem_id, 6);
		oem_id[6] = 0;

		txm_print(&earlytxm, "Found RSDP@ ");
		txm_print_hex(&earlytxm, (u64)rsdp);
		txm_print(&earlytxm, " OEM: ");
		txm_print(&earlytxm, oem_id);
		txm_line_feed(&earlytxm);
		txm_print(&earlytxm, "Found RSDT@ ");
		txm_print_hex(&earlytxm, rsdp->rsdt_physical_address);
		txm_print(&earlytxm, " (phys)");
		txm_line_feed(&earlytxm);

		// Zero extend to u64 and interpret as VA.
		// FIXME: this will probably stop working at some point, but
		// for now first 1GB of physical memory is mapped to our
		// virtual first 1GB.
		rsdt = (const acpi_table_rsdt*) ((u64) rsdp->rsdt_physical_address);

		if (rsdp->revision >= 2) {
			txm_print(&earlytxm, "Found XSDT@ ");
			txm_print_hex(&earlytxm, rsdp->xsdt_physical_address);
			txm_print(&earlytxm, " (phys)");
			txm_line_feed(&earlytxm);
		}
	} else {
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

	struct msr_res_t apic_msr = read_msr(APIC_BASE_MSR);
	txm_print(&earlytxm, "APIC_MSR: ");
	txm_print_hex(&earlytxm, (u64)(apic_msr.eax) | (((u64)apic_msr.edx) << 32));
	txm_line_feed(&earlytxm);

	const u64 apic_phys_base = apic_get_phys_base();
	txm_print(&earlytxm, "APIC PHYS BASE: ");
	txm_print_hex(&earlytxm, apic_phys_base);
	txm_line_feed(&earlytxm);

	// Map APIC to VM
	if (vmmap_4kb(&heap, pml4, (void*) APIC_BASE_VA, (void*) apic_phys_base)) {
		txm_print(&earlytxm, "mapping APIC failed with ");
		txm_print_hex(&earlytxm, ret);
		txm_line_feed(&earlytxm);
	}

	struct apic_info *apic_info = kalloc(&heap, sizeof(struct apic_info), 16);
	*apic_info = acpi_parse_madt(rsdt);
	txm_print(&earlytxm, "APIC IRQ base: ");
	txm_print_hex_u8(&earlytxm, apic_info->ioapic_irqbase);
	txm_line_feed(&earlytxm);

	txm_print(&earlytxm, "PML4: ");
	txm_print_hex(&earlytxm, (u64) pml4);
	txm_line_feed(&earlytxm);

	//setup_pic();
	//disable_pic();
	//apic_enable();

	///*
	// * Configure temporary APIC, just to see if interrupts are working
	// */

	//// Set the Spurious Interrupt Vector Register bit 8 to start receiving interrupts
	//apic_spv_t apic_spv;
	//apic_spv.as_u32 = apic_read(APIC_OFF_SPV);

	//apic_spv.enabled = 1;
	//apic_write(APIC_OFF_SPV, apic_spv.as_u32);

	//// Set clock divisor
	//const u32 apic_divisor =
	//	(apic_read(APIC_OFF_CLOCK_DIVISOR) & 0xfffffff0) | APIC_CLOCK_DIVISOR_BY_1;
	//apic_write(APIC_OFF_CLOCK_DIVISOR, apic_divisor);

	//apic_timer_t apic_timer;
	//apic_timer.as_u32 = apic_read(APIC_OFF_LVT_TIMER);

	//apic_timer.mode = APIC_TIMER_MODE_PERIODIC;
	//apic_timer.mask = APIC_TIMER_UNMASKED;
	//apic_timer.vector = IRQN_APIC_TIMER;
	//apic_write(APIC_OFF_LVT_TIMER, apic_timer.as_u32);
	//apic_write(APIC_OFF_TIMER_INIT_COUNT, 0);

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xa0
#define PIC2_DATA 0xa1

// end of intrrupte
#define PIC_EOI     0x20

#define ICW1_ICW4       0x01    /* ICW4 (not) needed */
#define ICW1_SINGLE     0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 (8) */
#define ICW1_LEVEL      0x08    /* Level triggered (edge) mode */
#define ICW1_INIT       0x10    /* Initialization - required! */
 
#define ICW4_8086       0x01    /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02    /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode/master */
#define ICW4_SFNM       0x10    /* Special fully nested (not) */

	outb(PIC1_CMD, ICW1_INIT + ICW1_ICW4);    // starts the initialization sequence (in cascade mode)
	outb(PIC2_CMD, ICW1_INIT + ICW1_ICW4);

	outb(PIC1_DATA, 0x20);                   // ICW2: Master PIC vector offset
	outb(PIC2_DATA, 0x28);                   // ICW2: Slave PIC vector offset

	outb(PIC1_DATA, 4);                      // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	outb(PIC2_DATA, 2);                      // ICW3: tell Slave PIC its cascade identity (0000 0010)

	outb(PIC1_DATA, ICW4_8086);
	outb(PIC2_DATA, ICW4_8086);

	outb(PIC1_DATA, 0x0);                   // clear irq maske, enable all irq in Mister PIC
	outb(PIC2_DATA, 0x0);                   // clear irq maske, enable all irq in Slave PIC

	__asm__ volatile ("sti\n");

	for(;;)
		__asm__ volatile ("hlt\n");
}

