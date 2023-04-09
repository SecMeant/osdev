#pragma once

static inline void barrier()
{
	__asm__ volatile(""::: "memory");
}

