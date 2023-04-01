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

u8 memcmp(const void *_p0, const void *_p1, u64 size)
{
	const u8 *p0 = _p0, *p1 = _p1;

	/* Don't do anything funky for now - memory might not be aligned. */
	for (u64 i = 0; i < size; ++i) {
		const u8 diff = *p0 - *p1;
		if (diff != 0)
			return diff;
	}
	
	return 0;
}
