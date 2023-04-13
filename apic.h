#pragma once

#include "compiler.h"
#include "msr.h"
#include "types.h"
#include "abort.h"

#define APIC_BASE_MSR 0x1B
#define APIC_BASE_MSR_BSP 0x100
#define APIC_BASE_MSR_ENABLE_BIT 0x800

#define APIC_OFF_SPV 0x0f0
#define APIC_OFF_LVT_CMCI 0x2f0
#define APIC_OFF_LVT_TIMER 0x320
#define APIC_OFF_LVT_THERMAL_MONITOR 0x330
#define APIC_OFF_PERFORMANCE_COUNTER 0x340
#define APIC_OFF_LINT0 0x350
#define APIC_OFF_LINT1 0x360
#define APIC_OFF_ERROR 0x370
#define APIC_OFF_TIMER_INIT_COUNT 0x380
#define APIC_OFF_TIMER_CURR_COUNT 0x390
#define APIC_OFF_CLOCK_DIVISOR 0x3e0

#define APIC_CLOCK_DIVISOR_BY_1   0b1011
#define APIC_CLOCK_DIVISOR_BY_2   0b0000
#define APIC_CLOCK_DIVISOR_BY_4   0b0001
#define APIC_CLOCK_DIVISOR_BY_8   0b0010
#define APIC_CLOCK_DIVISOR_BY_16  0b0011
#define APIC_CLOCK_DIVISOR_BY_32  0b1000
#define APIC_CLOCK_DIVISOR_BY_64  0b1001
#define APIC_CLOCK_DIVISOR_BY_128 0b1010

#define APIC_TIMER_MODE_ONESHOT  0
#define APIC_TIMER_MODE_PERIODIC 1
#define APIC_TIMER_MODE_DEADLINE 2

#define APIC_TIMER_UNMASKED 0
#define APIC_TIMER_MASKED   1

#define KERNEL_BASE_VA 0xc0000000UL
#define APIC_BASE_SIZE 0x1000UL

// Map APIC registers just before the kernel.
#define APIC_BASE_VA (KERNEL_BASE_VA - APIC_BASE_SIZE)

// APIC Spuious Vector Register (offset 0x0f0)
typedef struct {
	union {
		struct {
			// For the P6 family and Pentium processors, bits 0 through 3 are always 1.
			u32 vector: 8; // bits 0 - 7

			u32 enabled: 1; // bit 8

			// Not supported in Pentium 4 and Intel Xeon processors.
			u32 focus_cpu_check: 1; // bit 9

			u32 reserved0: 2; // bits 10 - 11

			// EOI-Broadcast suppresion
			// Not supported on all processors. See bit 24 of Local APIC Version Registe
			u32 eoi_broadcast_supp: 1; // bit 12

			u32 reserved1: 19; // bits 13 - 31
		};

		u32 as_u32;
	};
} apic_spv_t;
_Static_assert(sizeof(apic_spv_t) == sizeof(u32), "apic_spv_t sanity check");

// APIC LVT Timer (offset 0x320)
typedef struct {
	union {
		struct {
			u32 vector: 8;     // bits 0 - 7
			u32 reserved0: 4;  // bits 8 - 11
			u32 status : 1;    // bits 12
			u32 reserved1: 3;  // bits 13 - 15
			u32 mask: 1;       // bit 16
			u32 mode: 2;       // bit 17
			u32 reserved2: 13; // bits 19 - 31
		};

		u32 as_u32;
	};
} apic_timer_t;
_Static_assert(sizeof(apic_timer_t) == sizeof(u32), "apic_timer_t sanity check");

inline static u64 apic_get_phys_base(void)
{
	struct msr_res_t apic_msr = read_msr(APIC_BASE_MSR);

	return ((u64)(apic_msr.eax & 0xfffff000)) |
		(((u64)(apic_msr.edx & 0x0f)) << 32);
}

inline static void apic_enable(void)
{
	struct msr_res_t apic_base = read_msr(APIC_BASE_MSR);

	apic_base.eax |= APIC_BASE_MSR_ENABLE_BIT;

	write_msr(APIC_BASE_MSR, apic_base.eax, apic_base.edx);
}

// Read from the register at offset.
// Passing offset not aligned to 32 bits doesn't make any sense.
inline static u32 apic_read(u16 offset)
{
	if (offset % 4)
		panic();

	volatile u32 *reg = (u32*)(((char*) APIC_BASE_VA) + offset);
	const u32 ret = *reg;

	barrier();
	
	return ret;
}

// Write to the register at offset.
// Passing offset not aligned to 32 bits doesn't make any sense.
inline static void apic_write(u16 offset, u32 value)
{
	if (offset % 4)
		panic();

	volatile u32 *reg = (u32*)(((char*) APIC_BASE_VA) + offset);
	*reg = value;

	barrier();
}
