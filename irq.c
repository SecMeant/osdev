#include "irq.h"
#include "apic.h"
#include "io.h"
#include "kernel.h"
#include "textmode.h"
#include "types.h"

extern void irq_handler(void);
extern void irq_handler_keyboard(void);
extern void trap_handler(void);

IDTGD idt[256];

void load_idt(void)
{
	typedef struct {
		u16 limit;
		u64 base;
	} __attribute__((packed)) IDTR;

	// We need to offset address returned by GCC because we don't know
	// where bootloader loaded us. GCC assumes we are loaded at addr 0.
	u64 irq_handler_addr = ((u64) irq_handler) + boot_header.kernel_phys_base;
	u64 irq_handler_keyboard_addr = ((u64) irq_handler_keyboard) + boot_header.kernel_phys_base;

	// FIXME: We need proper address calculation, GCC doesn't know at what
	// address the kernel was loaded so the addresses are all fucked up
	// here.
	for (u64 i = 0; i < 256; ++i) {
		switch(i) {
			case 0x21:
				idt[i] = idtgd_set_64bit_offset(make_64bit_idtgd(), irq_handler_keyboard_addr);
				idt[i].type = GATE_TYPE_IRQ;
				break;

			default:
				idt[i] = idtgd_set_64bit_offset(make_64bit_idtgd(), irq_handler_addr);
				idt[i].type = GATE_TYPE_IRQ;
				break;
		}

		idt[i].present = 1;
		idt[i].segment_selector = 0x08;
		idt[i].ist = 0;
		idt[i].desc_priv_lvl = 0;
	}

	IDTR idtr;
	idtr.limit = sizeof(idt) - 1;
	idtr.base = (u64) &idt;

	__asm__ volatile (
		"lidt %[idtr]\n"
		: : [idtr] "m" (idtr)
	);
}

#define PIC_MASTER_COMMAND_PORT 0x0020
#define PIC_MASTER_DATA_PORT    0x0021
#define PIC_SLAVE_COMMAND_PORT  0x00A0
#define PIC_SLAVE_DATA_PORT     0x00A1

void setup_pic(void)
{
	// TODO: We probably have to wait between the outb instructions
	//       For now, it works in the QEMU so, whatever...
	outb(PIC_MASTER_COMMAND_PORT, 0x11);
	outb(PIC_MASTER_DATA_PORT, 0x20);
	outb(PIC_MASTER_DATA_PORT, 0x01);
}

void disable_pic(void)
{
	// TODO: We probably have to wait between the outb instructions
	//       For now, it works in the QEMU so, whatever...
	outb(PIC_MASTER_DATA_PORT, 0xff);
	outb(PIC_SLAVE_DATA_PORT,  0xff);
}

void setup_ioapic(const u32 ioapic_redir_entries)
{
	for (u32 i = 0; i < ioapic_redir_entries; ++i) {
		// Read redirection entry and remove mask bit.
		u32 redir_entry = ioapic_read(IOAPIC_BASE_VA, IOAPIC_REG_IOREDTBL(i));

		// Mask all, but keyboard irq
		const u32 mask_bit = 1u << 16u;
		if (i == 1)
			redir_entry &= (~mask_bit);
		else
			redir_entry |= mask_bit;

		// Level triggered
		redir_entry |= (1u << 15);

		// Vector number
		redir_entry |= 0x20 + i;

		ioapic_write(IOAPIC_BASE_VA, IOAPIC_REG_IOREDTBL(i), redir_entry);
	}
}

struct isr_context
{
	u64 r11;
	u64 r10;
	u64 r9;
	u64 r8;
	u64 rdx;
	u64 rcx;
	u64 rax;
	u64 rip;
	u64 cs;
	u64 rflags;
	u64 rsp;
	u64 ss;
};

void isr_default(struct isr_context *ctx)
{
	(void) ctx;
#if 0
	static int printed = 0;
	if (!printed) {
		txm_print(&earlytxm, "ISR CONTEXT: ");
		txm_line_feed(&earlytxm);

		txm_print(&earlytxm, "    R11: ");
		txm_print_hex(&earlytxm, ctx->r11);
		txm_line_feed(&earlytxm);

		txm_print(&earlytxm, "    R10: ");
		txm_print_hex(&earlytxm, ctx->r10);
		txm_line_feed(&earlytxm);

		txm_print(&earlytxm, "     R9: ");
		txm_print_hex(&earlytxm, ctx->r9);
		txm_line_feed(&earlytxm);

		txm_print(&earlytxm, "     R8: ");
		txm_print_hex(&earlytxm, ctx->r8);
		txm_line_feed(&earlytxm);

		txm_print(&earlytxm, "    RDX: ");
		txm_print_hex(&earlytxm, ctx->rdx);
		txm_line_feed(&earlytxm);

		txm_print(&earlytxm, "    RCX: ");
		txm_print_hex(&earlytxm, ctx->rcx);
		txm_line_feed(&earlytxm);

		txm_print(&earlytxm, "    RAX: ");
		txm_print_hex(&earlytxm, ctx->rax);
		txm_line_feed(&earlytxm);

		txm_print(&earlytxm, "    RIP: ");
		txm_print_hex(&earlytxm, ctx->rip);
		txm_line_feed(&earlytxm);

		txm_print(&earlytxm, "     CS: ");
		txm_print_hex(&earlytxm, ctx->cs);
		txm_line_feed(&earlytxm);

		txm_print(&earlytxm, "    FLG: ");
		txm_print_hex(&earlytxm, ctx->rflags);
		txm_line_feed(&earlytxm);

		txm_print(&earlytxm, "    RSP: ");
		txm_print_hex(&earlytxm, ctx->rsp);
		txm_line_feed(&earlytxm);
	}

	printed |= 1;
#endif

	// PIC EOI
	// outb(0x20, 0x20);
	
	// APIC EOI
	apic_write(0xB0, 0);
}

void isr_keyboard(struct isr_context *ctx)
{
	(void) ctx;

	txm_print(&earlytxm, "GOT KEYBOARD IRQ");
	txm_line_feed(&earlytxm);

	if ((inb(0x64) & 0x01) == 0)
	       return;

	u8 sc = inb(0x60);
	txm_print(&earlytxm, "SC: ");
	txm_print_hex_u8(&earlytxm, sc);
	txm_line_feed(&earlytxm);

	// PIC EOI
	//outb(0x20, 0x20);

	// APIC EOI
	apic_write(0xB0, 0);
}

