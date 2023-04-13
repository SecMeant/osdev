#pragma once

#include "types.h"

#define GPUBUF               0xb8000
#define GPUBUF_SCREEN_WIDTH  80
#define GPUBUF_SCREEN_HEIGHT 25
#define GPUBUF_CHAR_BG_BLACK 0x0f

typedef struct {
	u8 ch;
	u8 bg;
} __attribute__((packed)) TxmColor;

typedef TxmColor GpuTextBufRows[GPUBUF_SCREEN_WIDTH];
typedef GpuTextBufRows GpuTextBuf[GPUBUF_SCREEN_HEIGHT];

typedef struct {
	GpuTextBuf *mem;
	u64 curx;
	u64 cury;
} txmbuf;

extern txmbuf earlytxm;

void txm_putc(txmbuf *buf, char ch);
void txm_print(txmbuf *buf, char *s);
void txm_print_hex_u8(txmbuf *buf, u8 v);
void txm_print_hex(txmbuf *buf, u64 v);
void txm_line_feed(txmbuf *buf);
void txm_clear_cur_line(txmbuf *buf);
void txm_clear_screen(txmbuf *buf);
txmbuf make_early_txmbuf(void);

static inline void disable_cursor(void)
{
	__asm__ volatile (
		".intel_syntax noprefix\n"
		"mov dx, 0x03d4\n"
		"mov al, 0x0a\n"
		"outb dx, al\n"

		"inc dx\n"
		"mov al, 0x20\n"
		"out dx, al\n"
		".att_syntax\n"
	);
}
