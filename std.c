#include "types.h"

void memset(void *p_, u64 c, u64 n) 
{
	u8 *p = p_;

	while (n) {
		*p = c;

		++p;
		--n;
	}
}

void memcpy(void *dst_, const void *src_, u64 size)
{
	u8 *dst = dst_;
	const u8 *src = src_;

	while (size) {
		*dst = *src;

		++dst;
		++src;
		--size;
	}
}
