#pragma once

#include "types.h"

/* 4-Level PML4 Entry */
typedef struct {
	union {
		u64 as_u64;

		struct {
			u64 present       : 1;
			u64 writeable     : 1;
			u64 usermode      : 1;
			u64 write_through : 1;
			u64 cache_disable : 1;
			u64 accessed      : 1;
			u64 reserved_0    : 1;
			u64 reserved_1    : 1; // must be zero
			u64 reserved_2    : 4;
			u64 address       : 40; // has to be in cannonical form
			u64 reserved_3    : 11;
			u64 exec_disable  : 1;
		};
	};
} PML4E;

/* 4-Level Page-Directory-Pointer-Table Entry */
typedef struct {
	union {
		u64 as_u64;

		struct {
			u64 present       : 1;
			u64 writeable     : 1;
			u64 usermode      : 1;
			u64 write_through : 1;
			u64 cache_disable : 1;
			u64 accessed      : 1;
			u64 dirty         : 1;
			u64 size          : 1; // must be 0 for 4kb pages
			u64 reserved_2    : 4;
			u64 address       : 40; // has to be in cannonical form
			u64 reserved_3    : 11;
			u64 exec_disable  : 1;
		} pdpte_4kb;

		struct {
			u64 present       : 1;
			u64 writeable     : 1;
			u64 usermode      : 1;
			u64 write_through : 1;
			u64 cache_disable : 1;
			u64 accessed      : 1;
			u64 dirty         : 1;
			u64 size          : 1; // must be 1 for 1GB pages
			u64 global        : 1;
			u64 reserved_0    : 3;
			u64 pat           : 1;
			u64 reserved_1    : 17;
			u64 address       : 22; // has to be in cannonical form
			u64 reserved_2    : 7;
			u64 prot_key      : 4;
			u64 exec_disable  : 1;
		} pdpte_1gb;

		struct {
			u64 present       : 1;
			u64 writeable     : 1;
			u64 usermode      : 1;
			u64 write_through : 1;
			u64 cache_disable : 1;
			u64 accessed      : 1;
			u64 dirty         : 1;
			u64 is_1gb        : 1; // must be 1 for 1GB pages
			u64 reserved_0    : 4;
		};
	};
} PDPTE;

/* 4-Level Page-Directory Entry */
typedef union {
	u64 as_u64;

	struct {
		u64 present       : 1;
		u64 writeable     : 1;
		u64 usermode      : 1;
		u64 write_through : 1;
		u64 cache_disable : 1;
		u64 accessed      : 1;
		u64 reserved_0    : 1;
		u64 size          : 1; // must be 0 for 4kb pages
		u64 reserved_2    : 4;
		u64 address       : 40; // has to be in cannonical form
		u64 reserved_3    : 11;
		u64 exec_disable  : 1;
	};
} PDE;

/* 4-Level Page-Table Entry */
typedef union {
	u64 as_u64;

	struct  {
		u64 present       : 1;
		u64 writeable     : 1;
		u64 usermode      : 1;
		u64 write_through : 1;
		u64 cache_disable : 1;
		u64 accessed      : 1;
		u64 dirty         : 1;
		u64 pat           : 1;
		u64 global        : 1;
		u64 reserved_2    : 3;
		u64 address       : 40; // has to be in cannonical form
		u64 reserved_3    : 7;
		u64 prot_key      : 4;
		u64 exec_disable  : 1;
	};
} PTE;

_Static_assert(sizeof(PML4E) == 8);
_Static_assert(sizeof(PDPTE) == 8);
_Static_assert(sizeof(PDE) == 8);
_Static_assert(sizeof(PTE) == 8);

