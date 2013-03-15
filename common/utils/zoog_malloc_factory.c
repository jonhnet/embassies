#include "malloc_factory_operator_new.h"
#include "ambient_malloc.h"

#define USE_STOMP_DETECTOR 0

#include "zoog_malloc_factory.h"

void zoog_malloc_factory_init(
	ZoogMallocFactory *zmf, ZoogDispatchTable_v1 *xdt)
{
	concrete_mmap_xdt_init(&zmf->concrete_mmap_xdt_private, xdt);
#if USE_STOMP_DETECTOR
	stomp_init(&zmf->stomp_detector, &zmf->concrete_mmap_xdt_private.ami);
	zmf->cheesy_mmap_ifc = &stomp_new_region(&zmf->stomp_detector)->abstract_mmap_ifc;
	zmf->guest_mmap_ifc = &stomp_new_region(&zmf->stomp_detector)->abstract_mmap_ifc;
	zmf->brk_mmap_ifc = &stomp_new_region(&zmf->stomp_detector)->abstract_mmap_ifc;
#else
	zmf->cheesy_mmap_ifc = &zmf->concrete_mmap_xdt_private.ami;
	zmf->guest_mmap_ifc = &zmf->concrete_mmap_xdt_private.ami;
	zmf->brk_mmap_ifc = &zmf->concrete_mmap_xdt_private.ami;
#endif
	cheesy_malloc_init_arena(&zmf->cheesy_arena, zmf->cheesy_mmap_ifc);

	cmf_init(&zmf->cmf, &zmf->cheesy_arena);
	zmf->mf = &zmf->cmf.mf;

	malloc_factory_operator_new_init(zmf->mf);
	ambient_malloc_init(zmf->mf);
}

MallocFactory *zmf_get_mf(ZoogMallocFactory *zmf)
{
	return zmf->mf;
}
