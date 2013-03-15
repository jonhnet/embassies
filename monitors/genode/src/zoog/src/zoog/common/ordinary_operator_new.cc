#include <base/env.h>

// reach through to private defns in malloc_free.cc
extern "C" void *_private_malloc(unsigned size);
extern "C" void _private_free(void *ptr);

void *operator new(Genode::size_t size)
{
	return _private_malloc(size);
}

// Oddly, a delete is provided by cxx.lib, and it points at
// the same free that _private_free points at, so we omit that here. Odd.
