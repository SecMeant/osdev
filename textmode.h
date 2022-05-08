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
} TxmBuf;

void txm_putc(TxmBuf *buf, char ch);
void txm_print(TxmBuf *buf, char *s);
void txm_print_hex(TxmBuf *buf, u64 v);
void txm_line_feed(TxmBuf *buf);
void txm_clear_cur_line(TxmBuf *buf);
TxmBuf make_early_txmbuf(void);

