#include <dlfcn.h>
#include "MmapOverride.h"
#include "LiteLib.h"

static MmapOverride *g_override = NULL;

MmapOverride::MmapOverride()
{
	underlying_mmap = (mmap_f*) dlsym(RTLD_NEXT, "mmap");

	lite_assert(g_override == NULL);
	g_override = this;

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	dbg_lockheld = false;
}

void MmapOverride::lock()
{
	pthread_mutex_lock(&mutex);
	dbg_lockheld = true;
}

void MmapOverride::unlock()
{
	dbg_lockheld = false;
	pthread_mutex_unlock(&mutex);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	void *result;
	g_override->lock();
	result = (g_override->underlying_mmap)(addr, length, prot, flags, fd, offset);
	/*
	fprintf(stderr, "mmap override returns %08x len %08x\n", (uint32_t)result, length);
	*/
	g_override->unlock();
	return result;
}
