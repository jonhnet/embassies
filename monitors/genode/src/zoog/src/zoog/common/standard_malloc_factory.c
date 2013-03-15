#if ZOOG_NO_STANDARD_INCLUDES
void *_private_malloc(unsigned size);
void _private_free(void *ptr);
#else
#include <malloc.h>
#endif

#include "malloc_factory.h"

void *smf_malloc(MallocFactory *mf, size_t size)
{
	return _private_malloc(size);
}

void smf_free(MallocFactory *mf, void *ptr)
{
	_private_free(ptr);
}

static MallocFactory _standard_malloc_factory = { smf_malloc, smf_free };

#ifdef __cplusplus
extern "C" {
#endif
MallocFactory *standard_malloc_factory_init()
{
	return &_standard_malloc_factory;
}

void *ambient_malloc(size_t size) { return _private_malloc(size); }
void ambient_free(void *ptr) { return _private_free(ptr); }

#ifdef __cplusplus
}
#endif
