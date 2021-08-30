#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef u64 size_t;

#define COMPILE_BUG_ON(x) _Static_assert(!(x))

COMPILE_BUG_ON(sizeof(u8)  != 1);
COMPILE_BUG_ON(sizeof(u16) != 2);
COMPILE_BUG_ON(sizeof(u32) != 4);
COMPILE_BUG_ON(sizeof(u64) != 8);

