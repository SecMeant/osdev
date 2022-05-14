#include "memory.h"
#include "vm.h"
#include "types.h"
#include "compiler.h"
#include "abort.h"

#define EARLY_HEAP_BASE 0x40000000
#define CANNONICAL_ADDRESS_MASK 0x000fffffffffffff
#define PAGE_MASK_1GB 0xffffffffc0000000

#define IS_POWER_OF_2(x) ((x != 0) && ((x & (x - 1)) == 0))

static void memset(void *p_, u64 c, u64 n) 
{
	u8 *p = p_;

	while (n) {
		*p = c;

		++p;
		--n;
	}
}

void *kalloc(struct kernel_heap *kheap, u64 size, u64 alignment)
{
	void *ret = NULL;

	if (!IS_POWER_OF_2(alignment))
		return NULL;

	size = ((size + alignment - 1) / alignment) * alignment;

	if (kheap->head + size >= kheap->end)
		return NULL;

	ret = kheap->head;
	kheap->head += size;

	return ret;
}

static int vmmap_1gb(struct kernel_heap *heap, PML4E *pml4, void *virtual_, void *physical_)
{
	union {
		u64 as_u64;

		struct {
			u64 offset : 8;
			u64 page_addr : 22;
			u64 pdpte : 9;
			u64 pml4 : 9;
			u64 reserved : 16;
		};
	} page_parts;

	_Static_assert(sizeof(page_parts) == sizeof(u64));

	u64 physical = (u64) physical_;
	physical &= CANNONICAL_ADDRESS_MASK;
	physical &= PAGE_MASK_1GB;

	page_parts.as_u64 = (u64) virtual_;

	if (!pml4[page_parts.pml4].present) {
		if (!heap)
			return 1;

		const u64 pdpt_size = sizeof(PDPTE) * 512;
		PDPTE *pdpt = kalloc(heap, pdpt_size, 4096);

		if (!pdpt)
			return 1;

		memset(pdpt, 0, pdpt_size);

		pml4[page_parts.pml4].address = ((u64) pdpt) >> 12;
	}

	PDPTE *pdpt = (PDPTE*) (pml4[page_parts.pml4].address << 12);

	if (pdpt[page_parts.pdpte].present)
		return 1;

	pdpt[page_parts.pdpte].cache_disable = 1;
	pdpt[page_parts.pdpte].write_through = 1;
	pdpt[page_parts.pdpte].is_1gb = 1;
	pdpt[page_parts.pdpte].pdpte_1gb.address = physical >> 12;

	barrier();
	pdpt[page_parts.pdpte].present = 1;
	barrier();
	pml4[page_parts.pml4].present = 1;

	return 0;
}

static struct kernel_heap _make_heap(void *base, u64 size)
{
	struct kernel_heap ret;

	ret.begin = base;
	ret.head = base;
	ret.end = base + size;

	return ret;
}

struct kernel_heap make_early_heap(void *pml4_, const struct ram_info_entry *info, u64 info_size)
{
	void *phys_base = NULL;
	u64 size = 0;

	for (u64 i = 0; i < info_size; ++i) {
		if (info[i].type != RAM_INFO_TYPE_FREE)
			continue;

		if (info[i].size <= size)
			continue;

		phys_base = (void*) info[i].base;
		size = info[i].size;
	}

	PML4E *pml4 = pml4_;

	if (vmmap_1gb(NULL, pml4, (void*) EARLY_HEAP_BASE, phys_base))
		panic();

	struct kernel_heap ret = _make_heap((void*) EARLY_HEAP_BASE, size);
	return ret;
}
