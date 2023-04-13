#pragma once
#include "types.h"

#define GATE_TYPE_CALL 12
#define GATE_TYPE_IRQ  14
#define GATE_TYPE_TRAP 15

#define IRQN_APIC_TIMER 0x20

/* 64-bit IDT Gate Descriptor */
typedef struct
{
	u16 offset0_15;
	u16 segment_selector;
	u16 ist : 3;
	u16 reserved0 : 1;
	u16 reserved1 : 1;
	u16 reserved2 : 3;
	u16 type : 4;
	u16 reserved3 : 1;
	u16 desc_priv_lvl : 2;
	u16 present : 1;
	u16 offset16_31;
	u32 offset32_63;
	u32 reserved4;
} __attribute__((packed)) IDTGD;

_Static_assert(sizeof(IDTGD) == 16);

static inline IDTGD make_64bit_idtgd(void)
{
	IDTGD ret;

	// Manual memset so we don't have to include other headers. We also
	// count on compiler to optimize this away.
	for (u64 i = 0; i < sizeof(ret); ++i)
		*((u8*) &ret) = 0;

	ret.reserved0 = 0;
	ret.reserved1 = 0;
	ret.reserved2 = 0;
	ret.reserved3 = 0;

	return ret;
}

static inline IDTGD idtgd_set_64bit_offset(IDTGD desc, u64 offset_)
{
	union {
		u64 as_u64;

		struct {
			u64 offset0_15  : 16;
			u64 offset16_31 : 16;
			u64 offset32_63 : 16;
		};
	} offset;

	offset.as_u64 = offset_;

	desc.offset0_15 = offset.offset0_15;
	desc.offset16_31 = offset.offset16_31;
	desc.offset32_63 = offset.offset32_63;

	return desc;
}

static inline u64 idtgd_get_64bit_offset(IDTGD desc)
{
	union {
		u64 as_u64;

		struct {
			u64 offset0_15  : 16;
			u64 offset16_31 : 16;
			u64 offset32_63 : 16;
		};
	} offset;

	offset.offset0_15  =  desc.offset0_15;
	offset.offset16_31 =  desc.offset16_31;
	offset.offset32_63 =  desc.offset32_63;

	return offset.as_u64;
}

extern IDTGD idt[256];

void setup_pic(void);
void disable_pic(void);
void load_idt(void);

