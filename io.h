#pragma once
#include "types.h"

static inline void outb(u16 port, u8 data)
{
	__asm__ volatile (
		".intel_syntax noprefix\n"
		"outb %[port], %[data]\n"
		".att_syntax\n"
		: : [port] "d" (port), [data] "a" (data)
	);
}

static inline u8 inb(u16 port)
{
	u8 data;

	__asm__ volatile (
		".intel_syntax noprefix\n"
		"inb %[data], %[port]\n"
		".att_syntax\n"
		: [data] "=a" (data)
		: [port] "d" (port)
	);

	return data;
}
