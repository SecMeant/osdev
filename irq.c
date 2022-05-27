#include "types.h"
#include "irq.h"
#include "kernel.h"

#include "textmode.h"

extern void irq_handler(void);
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
	u64 irq_handler_addr = ((u64) irq_handler) + boot_header.kernel_base;

	// FIXME: We need proper address calculation, GCC doesn't know at what
	// address the kernel was loaded so the addresses are all fucked up
	// here.
	for (u64 i = 0; i < 256; ++i) {
		switch(i) {
			case 8:
			case 10:
			case 11:
			case 12:
			case 14:
			case 17:
			case 21:
			case 29:
			case 30:

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

static inline void outb(u16 port, u8 data)
{
	__asm__ volatile (
		".intel_syntax noprefix\n"
		"outb %[port], %[data]\n"
		".att_syntax\n"
		: : [port] "d" (port), [data] "a" (data)
	);
}

void setup_pic(void)
{
	outb(0x20, 0x11);
	outb(0x21, 0x20);
	outb(0x21, 0x01);
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

	outb(0x20, 0x20);
}
