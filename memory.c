#include "memory.h"
#include "types.h"

void *kalloc(struct kernel_heap *kheap, u64 size)
{
	void *ret = NULL;

	size = ((size + HEAP_ALIGNMENT - 1) / HEAP_ALIGNMENT) * HEAP_ALIGNMENT;

	if (kheap->head + size >= kheap->end)
		return NULL;

	ret = kheap->head;
	kheap->head += size;

	return ret;
}

struct kernel_heap make_early_heap(struct ram_info_entry *info, u64 info_size)
{
	void *base = NULL;
	u64 size = 0;

	for (u64 i = 0; i < info_size; ++i) {
		if (info[i].type != RAM_INFO_TYPE_FREE)
			continue;

		if (info[i].size <= size)
			continue;

		base = (void*) info[i].base;
		size = info[i].size;
	}

	struct kernel_heap ret = { .begin = base, .head = base, .end = base + size };
	return ret;
}
