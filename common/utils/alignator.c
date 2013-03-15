#include <stdint.h>

#include "LiteLib.h"
#include "alignator.h"

void *alignator_map(AbstractAlignedMmapIfc *mmapifc, size_t length, uint32_t alignment)
{
	Alignator *alignator = (Alignator *) mmapifc;
	uint32_t bloated_request =
		length + (alignment-1) + sizeof(AlignatorBackingRecord);
	void *ptr =
		(alignator->backing->mmap_f)(alignator->backing, bloated_request);
	void *aligned_ptr = (void*)
		((((uint32_t)ptr) + (alignment-1)) & (~(alignment-1)));
	AlignatorBackingRecord *abr =
		(AlignatorBackingRecord *) (aligned_ptr + length);
	lite_assert(((void*) (abr+1)) <= aligned_ptr + bloated_request);
	abr->backing_ptr = ptr;
	abr->backing_length = bloated_request;
	return aligned_ptr;
}

void alignator_unmap(AbstractAlignedMmapIfc *mmapifc, void *addr, size_t length)
{
	Alignator *alignator = (Alignator *) mmapifc;
	AlignatorBackingRecord *abr = (AlignatorBackingRecord *) (addr + length);
	(alignator->backing->munmap_f)(alignator->backing, abr->backing_ptr, abr->backing_length);
}

void alignator_init(Alignator *alignator, AbstractMmapIfc *backing)
{
	alignator->ami.mmap_f = alignator_map;
	alignator->ami.munmap_f = alignator_unmap;
	alignator->backing = backing;
}
