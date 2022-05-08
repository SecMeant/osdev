#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

_Static_assert(sizeof(u8) == 1);
_Static_assert(sizeof(u16) == 2);
_Static_assert(sizeof(u32) == 4);
_Static_assert(sizeof(u64) == 8);

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

_Static_assert(sizeof(i8) == 1);
_Static_assert(sizeof(i16) == 2);
_Static_assert(sizeof(i32) == 4);
_Static_assert(sizeof(i64) == 8);

