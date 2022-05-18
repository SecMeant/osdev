#include "textmode.h"

__attribute__((noreturn,noinline))
void panic()
{
	txmbuf term = make_early_txmbuf();

	txm_clear_screen(&term);
	txm_print(&term, "*** KERNEL PANIC ***");
	txm_line_feed(&term);

	txm_print(&term, "RIP: ");
	txm_print_hex(&term, (u64) __builtin_return_address(0));
	txm_line_feed(&term);

	u64 reg;
	__asm__ volatile (
		"lea (%%rsp), %[reg]\n"
		: [reg] "=r" (reg)
	);

	txm_print(&term, "RSP: ");
	txm_print_hex(&term, reg);
	txm_line_feed(&term);

	__asm__ volatile (
		"hlt\n"
	);

	while(1) {}
}

