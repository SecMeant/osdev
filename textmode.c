#include "textmode.h"

txmbuf earlytxm;

static void txm_scroll(txmbuf *buf, size_t line_count)
{
	if (line_count == 0)
		return;

	if (line_count >= GPUBUF_SCREEN_HEIGHT) {
		txm_clear_screen(buf);
		return;
	}

	for (int y = line_count; y < GPUBUF_SCREEN_HEIGHT; ++y) {
		for (u32 x = 0; x < GPUBUF_SCREEN_WIDTH; ++x) {
			(*buf->mem)[y-line_count][x] = (*buf->mem)[y][x];
		}
	}

	txm_clear_cur_line(buf);
}

void txm_putc(txmbuf *buf, char ch)
{
	(*buf->mem)[buf->cury][buf->curx].ch = ch;
	(*buf->mem)[buf->cury][buf->curx].bg = GPUBUF_CHAR_BG_BLACK;

	++buf->curx;
	if (buf->curx >= GPUBUF_SCREEN_WIDTH) {
		buf->curx = 0;
		++buf->cury;
		if (buf->cury >= GPUBUF_SCREEN_HEIGHT) {
			buf->cury = GPUBUF_SCREEN_HEIGHT - 1;
			txm_scroll(buf, 1);
		}
	}
}

void txm_clear_cur_line(txmbuf *buf)
{
	for (int i = 0; i < GPUBUF_SCREEN_WIDTH; ++i) {
		(*buf->mem)[buf->cury][i].ch = ' ';
		(*buf->mem)[buf->cury][i].bg = GPUBUF_CHAR_BG_BLACK;
	}
}

void txm_clear_screen(txmbuf *buf)
{
	for (int y = 0; y < GPUBUF_SCREEN_HEIGHT; ++y) {
		for (int x = 0; x < GPUBUF_SCREEN_WIDTH; ++x) {
			(*buf->mem)[y][x].ch = ' ';
			(*buf->mem)[y][x].bg = GPUBUF_CHAR_BG_BLACK;
		}
	}
}

void txm_print(txmbuf *buf, char *s)
{
	while (*s != 0) {
		txm_putc(buf, *s);
		++s;
	}
}

void txm_print_hex_u8(txmbuf *buf, u8 v)
{
	char digits[] = "0123456789ABCDEF";

	txm_putc(buf, digits[v >> 4]);
	txm_putc(buf, digits[v & 0x0f]);
}

void txm_print_hex(txmbuf *buf, u64 v)
{
	char digits[] = "0123456789ABCDEF";

	for (int i = 0; i < 16; ++i) {
		txm_putc(buf, digits[v >> 60]);
		v <<= 4;
	}
}

void txm_line_feed(txmbuf *buf)
{
	buf->curx = 0;
	++buf->cury;

	if (buf->cury >= GPUBUF_SCREEN_HEIGHT) {
		buf->cury = GPUBUF_SCREEN_HEIGHT - 1;
		txm_scroll(buf, 1);
	}
}

txmbuf make_early_txmbuf(void)
{
	txmbuf ret;
	ret.mem = (void*) GPUBUF;
	ret.cury = 0;
	ret.curx = 0;

	return ret;
}

