#pragma once

#include "types.h"

struct msr_res_t
{
	u32 eax;
	u32 edx;
};

static inline struct msr_res_t read_msr(u32 msr)
{
	struct msr_res_t ret;

	__asm__ (
		"rdmsr"
		: "=a" (ret.eax), "=d" (ret.edx)
		: "c" (msr)
	);

	return ret;
}

static inline void write_msr(u32 msr, u32 eax, u32 edx)
{
	__asm__ (
		"wrmsr"
		: : "a" (eax), "d" (edx), "c" (msr)
	);
}
