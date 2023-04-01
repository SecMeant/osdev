#pragma once
#include "types.h"

void memset(void *p_, u64 c, u64 n);
void memcpy(void *dst_, const void *src_, u64 size);
u8 memcmp(const void *p0, const void *p2, u64 size);

