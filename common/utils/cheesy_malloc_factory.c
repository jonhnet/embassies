#include "cheesy_malloc_factory.h"

void *cmf_malloc(MallocFactory *mf, size_t size)
{
	CheesyMallocFactory *cmf = (CheesyMallocFactory *) mf;
	return cheesy_malloc(cmf->cheesy_arena, size);
}

void cmf_free(MallocFactory *mf, void *ptr)
{
	// CheesyMallocFactory *cmf = (CheesyMallocFactory *) mf;
	cheesy_free(ptr);
}

void cmf_init(CheesyMallocFactory *cmf, CheesyMallocArena *cheesy_arena)
{
	cmf->mf.c_malloc = &cmf_malloc;
	cmf->mf.c_free = &cmf_free;
	cmf->cheesy_arena = cheesy_arena;
}

