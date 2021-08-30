#include "textmode.h"
#include "assert.h"

void txm_putc(TxmBuf *self, char c)
{
	assert(self->curx < GPUBUF_SCREEN_WIDTH);
	assert(self->cury < GPUBUF_SCREEN_HEIGHT);

	u16 v = GPUBUF_CHAR_BG_BLACK | c;
	(*self->buf)[self->cury][self->curx].v = v;

	++self->curx;
	if (self->curx >= GPUBUF_SCREEN_WIDTH) {
		self->curx = 0;
		++self->cury;
		if (self->cury >= GPUBUF_SCREEN_HEIGHT) {
			self->cury = 0;
		}
	}
}

static void clear_cur_line(TxmBuf *self)
{
	for (int i = 0; i < GPUBUF_SCREEN_WIDTH; ++i) {
		u16 v = GPUBUF_CHAR_BG_BLACK | ' ';
		(*self->buf)[self->cury][i].v = v;
	}
}

static void line_feed(TxmBuf *self)
{
	self->curx = 0;
	++self->cury;

	if (self->cury >= GPUBUF_SCREEN_HEIGHT)
		self->cury = 0;
}

void txm_puts(TxmBuf *self, const char *s)
{
	clear_cur_line(self);

	while (*s) {
		txm_putc(self, *s);
		++s;
	}

	line_feed(self);
}

TxmBuf make_early_txmbuf()
{
	TxmBuf ret = { .buf = (TxmColorArray*) GPUBUF, .curx = 0, .cury = 3 };
	return ret;
}

