#include "memory.h"
#include "vm.h"
#include "types.h"
#include "compiler.h"
#include "abort.h"

// 1 MB of early heap
#define EARLY_HEAP_SIZE (1024ull * 1024ull)
static char EARLY_HEAP_MEM[EARLY_HEAP_SIZE];

#define CANNONICAL_ADDRESS_MASK 0x000fffffffffffff
#define PAGE_MASK_1GB 0xffffffffc0000000
#define PAGE_MASK_4KB 0xfffffffffffff000

#define IS_POWER_OF_2(x) ((x != 0) && ((x & (x - 1)) == 0))

void memset(void *p_, u64 c, u64 n) 
{
	u8 *p = p_;

	while (n) {
		*p = c;

		++p;
		--n;
	}
}

void memcpy(void *dst_, const void *src_, u64 size)
{
	u8 *dst = dst_;
	const u8 *src = src_;

	while (size) {
		*dst = *src;

		++dst;
		++src;
		--size;
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

void *kzalloc(struct kernel_heap *kheap, u64 size, u64 alignment)
{
	void *ret = kalloc(kheap, size, alignment);

	if (ret)
		memset(ret, 0, size);

	return ret;
}

// TODO: Add different exit codes for pages that cannot be mapped for some
//       reason and pages that are already mapped.
int vmmap_4kb(struct kernel_heap *heap, PML4E *pml4, void *virtual_, void *physical_)
{
	u64 physical = (u64) physical_;

	union {
		u64 as_u64;

		struct {
			u64 offset   : 12;
			u64 pte      : 9;
			u64 pde      : 9;
			u64 pdpte    : 9;
			u64 pml4e    : 9;
			u64 reserved : 16;
		};
	} page_parts;

	_Static_assert(sizeof(page_parts) == sizeof(u64));

	physical &= CANNONICAL_ADDRESS_MASK;
	physical &= PAGE_MASK_4KB;

	page_parts.as_u64 = (u64) virtual_;

	// TODO: make sure these writes happen (with atomic / volatile).
	//       for now we are leaving this as is - the probability that
	//       compiler will generate here a code that ommits the writes
	//       is quite low ;].
	PML4E *pml4e = &pml4[page_parts.pml4e];
	PDPTE *pdpt = NULL;
	PDE *pd = NULL;
	PTE *pt = NULL;

	if (!pml4e->present) {

		if (!heap)
			return 1;

		pdpt = kzalloc(heap, 4096, 4096);

		pml4e->cache_disable = 1;
		pml4e->write_through = 1;
		pml4e->writeable = 1;
		pml4e->address = ((u64) pdpt >> 12);
	}

	pdpt = (PDPTE*) ((u64) pml4e->address << 12lu);
	PDPTE *pdpte = &pdpt[page_parts.pdpte];

	if (!pdpte->present) {
		pd = kzalloc(heap, 4096, 4096);

		pdpte->cache_disable = 1;
		pdpte->write_through = 1;
		pdpte->writeable = 1;
		pdpte->pdpte_4kb.address = ((u64) pd >> 12);
	}

	// TODO: Add handling for 2MB pages, for now we don't care.
	if (pdpte->is_1gb)
		return 1;

	pd = (PDE*) ((u64) pdpte->pdpte_4kb.address << 12);
	PDE *pde = &pd[page_parts.pde];

	if (!pde->present) {
		pt = kzalloc(heap, 4096, 4096);

		pde->cache_disable = 1;
		pde->write_through = 1;
		pde->writeable = 1;
		pde->address = ((u64) pt >> 12);
	}

	pt = (PTE*) ((u64) pde->address << 12);
	PTE *pte = &pt[page_parts.pte];

	if (pte->present)
		// TODO: we should probably free allocated hierarchy along the way.
		return 1;

	pte->cache_disable = 1;
	pte->write_through = 1;
	pte->writeable = 1;
	pte->address = physical >> 12;

	barrier();
	pte->present = 1;
	barrier();
	pde->present = 1;
	barrier();
	pdpte->present = 1;
	barrier();
	pml4e->present = 1;

	return 0;
}

int vmmap_1gb(struct kernel_heap *heap, PML4E *pml4, void *virtual_, void *physical_)
{
	union {
		u64 as_u64;

		struct {
			u64 offset   : 30;
			u64 pdpte    : 9;
			u64 pml4e    : 9;
			u64 reserved : 16;
		};
	} page_parts;

	_Static_assert(sizeof(page_parts) == sizeof(u64));

	u64 physical = (u64) physical_;
	physical &= CANNONICAL_ADDRESS_MASK;
	physical &= PAGE_MASK_1GB;

	page_parts.as_u64 = (u64) virtual_;

	if (!pml4[page_parts.pml4e].present) {
		if (!heap)
			return 1;

		const u64 pdpt_size = sizeof(PDPTE) * 512;
		PDPTE *pdpt = kzalloc(heap, pdpt_size, 4096);

		if (!pdpt)
			return 1;

		pml4[page_parts.pml4e].address = ((u64) pdpt) >> 12;
	}

	PDPTE *pdpt = (PDPTE*) ((u64) pml4[page_parts.pml4e].address << 12);

	if (pdpt[page_parts.pdpte].present)
		return 1;

	pdpt[page_parts.pdpte].cache_disable = 1;
	pdpt[page_parts.pdpte].write_through = 1;
	pdpt[page_parts.pdpte].writeable = 1;
	pdpt[page_parts.pdpte].is_1gb = 1;
	pdpt[page_parts.pdpte].pdpte_1gb.address = physical >> 12;

	barrier();
	pdpt[page_parts.pdpte].present = 1;
	barrier();
	pml4[page_parts.pml4e].present = 1;

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

struct kernel_heap make_early_heap()
{
	struct kernel_heap ret = _make_heap((void*) EARLY_HEAP_MEM, EARLY_HEAP_SIZE);
	return ret;
}
