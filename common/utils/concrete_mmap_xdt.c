#include "concrete_mmap_xdt.h"

void *concrete_mmap_xdt_mmap(AbstractMmapIfc *ami, size_t length)
{
	ConcreteMmapXdt *cmx = (ConcreteMmapXdt *) ami;
	void *region = (cmx->zdt->zoog_allocate_memory)(length);
	return region;
}

void concrete_mmap_xdt_munmap(AbstractMmapIfc *ami, void *addr, size_t length)
{
	ConcreteMmapXdt *cmx = (ConcreteMmapXdt *) ami;
	(cmx->zdt->zoog_free_memory)(addr, length);
}

void concrete_mmap_xdt_init(ConcreteMmapXdt *cmx, ZoogDispatchTable_v1 *zdt)
{
	cmx->ami.mmap_f = concrete_mmap_xdt_mmap;
	cmx->ami.munmap_f = concrete_mmap_xdt_munmap;
	cmx->zdt = zdt;
}

