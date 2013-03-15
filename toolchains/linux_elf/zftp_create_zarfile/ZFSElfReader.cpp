#include "linked_list.h"
#include "malloc_factory.h"
#include "standard_malloc_factory.h"

#include "ZFSElfReader.h"

typedef struct {
	ElfReaderIfc ifc;
	ZFSReader* zf;
	MallocFactory *mf;
	LinkedList bufs_to_free;
} ZFSElfReader;

bool _ezfs_read(ElfReaderIfc *eri, uint8_t *buf, uint32_t offset, uint32_t length)
{
	ZFSElfReader *zer = (ZFSElfReader *) eri;

	uint32_t filelen;
	bool brc = zer->zf->get_filelen(&filelen);
	lite_assert(brc);

	if (offset+length > filelen)
	{
		return false;
	}
	uint32_t rc = zer->zf->get_data(offset, buf, length);
	if (rc!=length)
	{
		return false;
	}
	return true;
}

void *_ezfs_read_alloc(ElfReaderIfc *eri, uint32_t offset, uint32_t length)
{
	ZFSElfReader *zer = (ZFSElfReader *) eri;
	uint8_t *mem = (uint8_t*) mf_malloc(zer->mf, length);
	bool rc = _ezfs_read(&zer->ifc, mem, offset, length);
	if (!rc)
	{
		mf_free(zer->mf, mem);
		return NULL;
	}
	linked_list_insert_tail(&zer->bufs_to_free, mem);
	return mem;
}

void _ezfs_dtor(ElfReaderIfc *eri)
{
	ZFSElfReader *zer = (ZFSElfReader *) eri;
	while (zer->bufs_to_free.count > 0)
	{
		uint8_t *mem = (uint8_t*) linked_list_remove_head(&zer->bufs_to_free);
		mf_free(zer->mf, mem);
	}
	delete zer->zf;
}

ElfReaderIfc *zfs_elf_reader_init(ZFSReader* zf)
{
	MallocFactory* mf = standard_malloc_factory_init();
	ZFSElfReader *zer = (ZFSElfReader *) mf_malloc(mf, sizeof(ZFSElfReader));
	zer->ifc.elf_read = _ezfs_read;
	zer->ifc.elf_read_alloc = _ezfs_read_alloc;
	zer->ifc.elf_dtor = _ezfs_dtor;
	zer->zf = zf;
	zer->mf = mf;
	linked_list_init(&zer->bufs_to_free, mf);
	return &zer->ifc;
}

