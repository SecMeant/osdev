#include "types.h"
#include "textmode.h"

int kmain(const void *kbase, void *pml4)
{
	TxmBuf term = make_early_txmbuf();

	txm_print(&term, "Kernel successfuly loaded from ELF!");
	txm_line_feed(&term);

	txm_print(&term, "Kernel relocated @ 0x");
	txm_print_hex(&term, (u64) kbase);
	txm_line_feed(&term);

	txm_print(&term, "PML4 allocated   @ 0x");
	txm_print_hex(&term, (u64) pml4);
	txm_line_feed(&term);

	while (1) {}
}
