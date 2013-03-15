#include <stdio.h>
#include <sys/mman.h>
#include "concrete_mmap_posix.h"
#include "LiteLib.h"

void *concrete_mmap_posix_mmap(AbstractMmapIfc *ami, size_t length)
{
	void *region = mmap(
		NULL, length, PROT_EXEC|PROT_READ|PROT_WRITE,
		MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	lite_assert(region!=MAP_FAILED);
	return region;
}

void concrete_mmap_posix_munmap(AbstractMmapIfc *ami, void *addr, size_t length)
{
	munmap(addr, length);
}

void concrete_mmap_posix_init(ConcreteMmapPosix *cmp)
{
	cmp->ami.mmap_f = concrete_mmap_posix_mmap;
	cmp->ami.munmap_f = concrete_mmap_posix_munmap;
}

