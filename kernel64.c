#include "textmode.h"

void _start(void)
{
	TxmBuf buf = make_early_txmbuf();

	txm_puts(&buf, "Kernel successfuly loaded from ELF!");

	while (1) {}
}