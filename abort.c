#include "textmode.h"

__attribute__((noreturn,noinline))
void panic(void)
{
	txmbuf term = make_early_txmbuf();

	txm_clear_cur_line(&term);
	txm_putc(&term, '*');
	txm_line_feed(&term);

	txm_clear_cur_line(&term);
	txm_print(&term, "* KERNEL PANIC");
	txm_line_feed(&term);

	txm_clear_cur_line(&term);
	txm_putc(&term, '*');
	txm_line_feed(&term);

	txm_clear_cur_line(&term);
	txm_print(&term, "RIP: ");
	txm_print_hex(&term, (u64) __builtin_return_address(0));
	txm_line_feed(&term);

	u64 reg;
	__asm__ volatile (
		"lea (%%rsp), %[reg]\n"
		: [reg] "=r" (reg)
	);

	txm_clear_cur_line(&term);
	txm_print(&term, "RSP: ");
	txm_print_hex(&term, reg);
	txm_line_feed(&term);

	txm_clear_cur_line(&term);
	for (int i = 0; i < GPUBUF_SCREEN_WIDTH / 2; ++i)
		txm_putc(&term, '*');

	txm_line_feed(&term);
	txm_clear_cur_line(&term);

	while(1) {
		__asm__ volatile (
			"cli\n"
			"hlt\n"
		);
	}
}

