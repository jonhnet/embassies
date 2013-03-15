#include "LiteLib.h"
#include "page_allocator.h"
#include "cheesylock.h"
#include "lite_log2.h"

#define PA_BIN_MAGIC	0xb1b1b1b1
#define PA_FREE_MAGIC	0xfef0f1f3

inline static uint32_t round_down(uint32_t val, uint32_t align)
{
	return (val & (~(align-1)));
}

void page_allocator_init_arena(PageAllocatorArena *arena, AbstractAlignedMmapIfc *mmapifc)
{
	int bin_idx;

	cheesy_lock_init(&arena->mutex);
	int num_bins = sizeof(arena->binHead)/sizeof(arena->binHead[0]);
	for (bin_idx=0; bin_idx<num_bins; bin_idx+=1)
	{
		arena->binHead[bin_idx] = NULL;
	}
	arena->mmapifc = mmapifc;
}

PageAllocatorBin *page_allocator_new_bin(PageAllocatorArena *arena, int bin_idx)
{
	uint32_t space_needed;
	void *space;
	PageAllocatorBin *bin;
	int slot;
	void *first_region;
	uint32_t offset;
	void *ptr;

	// Capacity should be the largest number such that
	// the round_down(offset [capacity-1]) == 0
	// That is, PA_PAGE_SIZE + (capacity-1)*(1<<bin_idx) < PA_CHUNK_SIZE
	// So: capacity < ((PA_CHUNK_SIZE - PA_PAGE_SIZE) / (1<<bin_idx))+1
	int capacity = ((PA_CHUNK_SIZE - PA_PAGE_SIZE - 1) >> bin_idx)+1;
	lite_assert(round_down(PA_PAGE_SIZE + (capacity-1)*(1<<bin_idx), PA_CHUNK_SIZE)==0);

	lite_assert(PA_LG_PAGE_SIZE<=bin_idx);

	space_needed = (PA_PAGE_SIZE) + capacity * (1<<bin_idx);
	space = (arena->mmapifc->mmap_f)
		(arena->mmapifc, space_needed, PA_CHUNK_SIZE);

	// assert that we got memory aligned as expected.
	lite_assert((((uint32_t)space) & (PA_CHUNK_SIZE-1))==0);

	bin = (PageAllocatorBin*) space;
	bin->magic = PA_BIN_MAGIC;
	bin->arena = arena;
	bin->lg_alloc_size = bin_idx;
	bin->capacity = capacity;
	bin->count = 0;
	bin->next = NULL;
	first_region = ((void*) bin) + (1<<(PA_LG_PAGE_SIZE));
	for (slot = 0; slot<bin->capacity; slot++)
	{
		bin->occupied[slot] = false;
		offset = (1<<bin->lg_alloc_size) * slot;
		ptr = first_region + offset;
		*((uint32_t*)(ptr)) = PA_FREE_MAGIC;
	}
	return bin;
}

void *page_alloc(PageAllocatorArena *arena, uint32_t length)
{
	uint32_t internal_length;
	uint32_t bin_idx;
	PageAllocatorBin **bin_ptr;
	PageAllocatorBin *bin;
	int slot;
	void *first_region;
	uint32_t offset;
	void *ptr;

	lite_assert(length > 0);

	cheesy_lock_acquire(&arena->mutex);

	internal_length = length;
	if (internal_length < (1<<PA_LG_PAGE_SIZE))
	{
		internal_length = (1<<PA_LG_PAGE_SIZE);
	}

	bin_idx = lite_log2(internal_length-1);
	lite_assert(internal_length <= (1<<bin_idx));
	lite_assert(1<<(bin_idx-1) < internal_length);

	bin_ptr = &arena->binHead[bin_idx];
	while (true)
	{
		if (*bin_ptr == NULL)
		{
			*bin_ptr = page_allocator_new_bin(arena, bin_idx);
		}

		lite_assert((*bin_ptr)->magic == PA_BIN_MAGIC);

		if ((*bin_ptr)->count == (*bin_ptr)->capacity)
		{
			bin_ptr = &((*bin_ptr)->next);
			continue;
		}

		// found/made a bin with empty slots.
		break;
	}

	bin = (*bin_ptr);

	lite_assert(bin->lg_alloc_size == bin_idx);

	for (slot=0; slot<bin->capacity; slot++)
	{
		if (!bin->occupied[slot])
		{
			break;
		}
	}
	lite_assert(slot < bin->capacity);	// else bin->count<bin->capacity is a lie!
	bin->count+=1;
	bin->occupied[slot] = true;

	first_region = ((void*) bin) + (1<<(PA_LG_PAGE_SIZE));
	offset = (1<<bin->lg_alloc_size) * slot;
	ptr = first_region + offset;
	lite_assert(*((uint32_t*)(ptr))==PA_FREE_MAGIC);
	*((uint32_t*)(ptr)) = -1;

	cheesy_lock_release(&arena->mutex);
	return ptr;
}

void page_free(void *ptr)
{
	PageAllocatorBin *bin;
	PageAllocatorArena *arena;
	uint32_t offset;
	uint32_t index;

	bin = (PageAllocatorBin *) round_down((uint32_t) ptr, PA_CHUNK_SIZE);
	arena = bin->arena;

	cheesy_lock_acquire(&arena->mutex);
	lite_assert(bin->magic == PA_BIN_MAGIC);
	offset = ptr - (void*) (((void*)bin)+(1<<(PA_LG_PAGE_SIZE)));
	index = offset >> bin->lg_alloc_size;
	lite_assert(index<<bin->lg_alloc_size == offset);
	lite_assert(index>=0 && index<bin->capacity);
	bin->occupied[index] = false;
	bin->count -= 1;
	((uint32_t*) ptr)[0] = PA_FREE_MAGIC;

	if (bin->count==0)
	{
		// TODO release completely-empty allocators. Need a linked list.
	}

	cheesy_lock_release(&arena->mutex);
}
