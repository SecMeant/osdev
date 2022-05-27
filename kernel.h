#pragma once

#include "types.h"
#include "memory.h"

struct kernel_boot_header
{
	u64 kernel_base;
	u64 kernel_end;
	u64 pml4;

	// We use it to make ram_info array aligned to 16 bytes. This makes
	// some code slightly easier/shorter in bootloader when detecting
	// memory.
	u64 reserved; 

	struct ram_info_entry ram_info[64];
	u64 ram_info_entries;
};

extern struct kernel_boot_header boot_header;
