#include "textmode.h"

void txm_putc(TxmBuf *buf, char ch)
{
	(*buf->mem)[buf->cury][buf->curx].ch = ch;
	(*buf->mem)[buf->cury][buf->curx].bg = GPUBUF_CHAR_BG_BLACK;

	++buf->curx;
	if (buf->curx >= GPUBUF_SCREEN_WIDTH) {
		buf->curx = 0;
		++buf->cury;
		if (buf->cury >= GPUBUF_SCREEN_HEIGHT) {
			buf->cury = 0;
		}
	}
}

void txm_clear_cur_line(TxmBuf *buf)
{
	for (int i = 0; i < GPUBUF_SCREEN_WIDTH; ++i) {
		(*buf->mem)[buf->cury][i].ch = ' ';
		(*buf->mem)[buf->cury][i].bg = GPUBUF_CHAR_BG_BLACK;
	}
}

void txm_print(TxmBuf *buf, char *s)
{
	txm_clear_cur_line(buf);

	while (*s != 0) {
		txm_putc(buf, *s);
		++s;
	}
}

void txm_print_hex(TxmBuf *buf, u64 v)
{
	char digits[] = "0123456789ABCDEF";

	for (int i = 0; i < 16; ++i) {
		txm_putc(buf, digits[v >> 60]);
		v <<= 4;
	}
}

void txm_line_feed(TxmBuf *buf)
{
	buf->curx = 0;
	++buf->cury;

	if (buf->cury >= GPUBUF_SCREEN_HEIGHT)
		buf->cury = 0;
}

TxmBuf make_early_txmbuf(void)
{
	TxmBuf ret;
	ret.mem = (void*) GPUBUF;
	ret.cury = 3;
	ret.curx = 0;

	return ret;
}

