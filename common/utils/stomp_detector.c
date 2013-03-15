#include "stomp_detector.h"
#include "xax_util.h"

typedef struct {
	uint32_t base;
	size_t end;
} StompAllocationRec;

#if 0
uint32_t stomp_hash_func(void *v_a)
{
	StompAllocationRec *sar_a = (StompAllocationRec *) v_a;
	return sar_a->base;
}
#endif

int stomp_cmp_func(const void *v_a, const void *v_b)
{
	StompAllocationRec *sar_a = (StompAllocationRec *) v_a;
	StompAllocationRec *sar_b = (StompAllocationRec *) v_b;
	return sar_a->base - sar_b->base;
}

void stomp_init(StompDetector *sd, AbstractMmapIfc *raw_storage)
{
	sd->raw_storage = raw_storage;
	cheesy_malloc_init_arena(&sd->cma, raw_storage);
	cmf_init(&sd->cmf, &sd->cma);
	sd->mf = &sd->cmf.mf;
	ordered_collection_init(&sd->oc, sd->mf, stomp_cmp_func);
}

void *stomp_allocate(AbstractMmapIfc *mmapifc, size_t length)
{
	StompRegion *sr = (StompRegion *) mmapifc;
	StompDetector *sd = sr->sd;
	void *new_region = (sd->raw_storage->mmap_f)(sd->raw_storage, length);
	StompAllocationRec *sar =
		(StompAllocationRec *) mf_malloc(sd->mf, sizeof(StompAllocationRec));
	sar->base = (uint32_t) new_region;
	sar->end = sar->base + length;

	xax_assert(sar->base < sar->end);
	ordered_collection_insert(&sd->oc, sar);
	return new_region;
}

void stomp_free(AbstractMmapIfc *mmapifc, void *addr, size_t length)
{
	StompRegion *sr = (StompRegion *) mmapifc;
	StompDetector *sd = sr->sd;

	uint32_t uaddr = (uint32_t) addr;
	uint32_t uend = uaddr + length;
	StompAllocationRec key;
	key.base = uaddr;
	StompAllocationRec *sar =
		(StompAllocationRec *) ordered_collection_lookup_le(&sd->oc, &key);
	ordered_collection_remove(&sd->oc, sar);
	
	if (sar->base != uaddr)
	{
		xax_assert(sar->base < uaddr);
		xax_assert(sar->end == uend);
			// we only handle trims from one end or the other.
		// this free snipped off the end.
		sar->end = uaddr;
	}
	else if (sar->end != uend)
	{
		// this free snipped off the beginning
		xax_assert(uend < sar->end);
		// trimmed end
		sar->base = uend;
	}
	else
	{
		// deleted
		mf_free(sd->mf, sar);
		sar = NULL;
	}
	if (sar!=NULL)
	{
		xax_assert(sar->base < sar->end);
		// still some left. Put it back.
		ordered_collection_insert(&sd->oc, sar);
	}

	// NB we remove the stomp record before we release the memory,
	// to avoid a race where another thread claims the same region,
	// and tries to insert the record before we've removed our old
	// one.
	(sd->raw_storage->munmap_f)(sd->raw_storage, addr, length);
}

StompRegion *stomp_new_region(StompDetector *sd)
{
	StompRegion *sr = mf_malloc(sd->mf, sizeof(StompRegion));
	sr->abstract_mmap_ifc.mmap_f = stomp_allocate;
	sr->abstract_mmap_ifc.munmap_f = stomp_free;
	sr->sd = sd;
	return sr;
}
