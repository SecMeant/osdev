#pragma once
#include "types.h"

#define GPUBUF               0xb8000ULL
#define GPUBUF_SCREEN_WIDTH  80
#define GPUBUF_SCREEN_HEIGHT 25
#define GPUBUF_CHAR_BG_BLACK 0x0f00

typedef struct __attribute__((packed))
{
	union
	{
		struct
		{
			u8 ch;
			u8 bg;
		};

		u16 v;
	};
} TxmColor;

typedef volatile TxmColor TxmColorArray[GPUBUF_SCREEN_HEIGHT][GPUBUF_SCREEN_WIDTH];

typedef struct
{
	TxmColorArray *buf;
	size_t curx;
	size_t cury;
} TxmBuf;

void txm_putc(TxmBuf *self, char c);
void txm_puts(TxmBuf *self, const char *s);
TxmBuf make_early_txmbuf();

