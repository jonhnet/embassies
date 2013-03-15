#include "ambient_malloc.h"

static MallocFactory *g_malloc_factory_for_ambient_malloc = NULL;

void ambient_malloc_init(MallocFactory *mf)
{
	g_malloc_factory_for_ambient_malloc = mf;
}

void *ambient_malloc(size_t size)
{
	return mf_malloc(g_malloc_factory_for_ambient_malloc, size);
}

void ambient_free(void *ptr)
{
	mf_free(g_malloc_factory_for_ambient_malloc, ptr);
}

