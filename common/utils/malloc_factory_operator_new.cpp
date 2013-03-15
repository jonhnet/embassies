#include "pal_abi/pal_types.h"
#include "malloc_factory_operator_new.h"

static MallocFactory *g_malloc_factory_for_new;

void malloc_factory_operator_new_init(MallocFactory *mf)
{
	g_malloc_factory_for_new = mf;
}

void *operator new(std::size_t size)
{
	return mf_malloc(g_malloc_factory_for_new, size);
}

void operator delete(void *ptr)
{
	if (ptr!=NULL)
	{
		mf_free(g_malloc_factory_for_new, ptr);
	}
}
