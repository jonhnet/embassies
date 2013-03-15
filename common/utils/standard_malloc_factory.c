#if ZOOG_NO_STANDARD_INCLUDES
void *malloc(unsigned size);
void free(void *ptr);
#else
#include <malloc.h>
#endif

#include "malloc_factory.h"

void *smf_malloc(MallocFactory *mf, size_t size)
{
	return malloc(size);
}

void smf_free(MallocFactory *mf, void *ptr)
{
	free(ptr);
}

static MallocFactory _standard_malloc_factory = { smf_malloc, smf_free };

#ifdef __cplusplus
extern "C" {
#endif
MallocFactory *standard_malloc_factory_init()
{
	return &_standard_malloc_factory;
}
#ifdef __cplusplus
}
#endif
