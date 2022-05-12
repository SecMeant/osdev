#pragma once
#include "types.h"


/*
 * Early RAM detection
 */

enum ram_info_type_e
{
	RAM_INFO_TYPE_FREE     = 1,
	RAM_INFO_TYPE_RESERVED = 2,
};

struct ram_info_entry
{
	u64 base;
	u64 size;
	u32 type;
	u32 attr; // ACPI 3.0 extended attribute bitfield
};

_Static_assert(
	sizeof(struct ram_info_entry) == 24,
	"Unexpected ram_info_entry size"
);


/*
 * Kernel heap utilities
 */

#define HEAP_ALIGNMENT 8

struct kernel_heap {
	void *begin;
	void *head;
	void *end;
};

void *kalloc(struct kernel_heap *kheap, u64 size);
static inline void kfree(struct kernel_heap *kheap, void *p)
{
	(void) kheap;
	(void) p;
}

struct kernel_heap make_early_heap(struct ram_info_entry *info, u64 info_size);

