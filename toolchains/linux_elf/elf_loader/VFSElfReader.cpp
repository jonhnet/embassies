#include "linked_list.h"
#include "malloc_factory.h"

#include "VFSElfReader.h"

typedef struct {
	ElfReaderIfc ifc;
	XaxVFSHandleIfc *hdl;
	MallocFactory *mf;
	LinkedList bufs_to_free;
} VFSElfReader;

bool _ver_read(ElfReaderIfc *eri, uint8_t *buf, uint32_t offset, uint32_t length)
{
	VFSElfReader *ver = (VFSElfReader *) eri;
	if (offset+length > ver->hdl->get_file_len())
	{
		return false;
	}
	XfsErr err;
	ver->hdl->read(&err, buf, length, offset);
	return (err == XFS_NO_ERROR);
}

void *_ver_read_alloc(ElfReaderIfc *eri, uint32_t offset, uint32_t length)
{
	VFSElfReader *ver = (VFSElfReader *) eri;
	uint8_t *mem = (uint8_t*) mf_malloc(ver->mf, length);
	bool rc = _ver_read(&ver->ifc, mem, offset, length);
	if (!rc)
	{
		mf_free(ver->mf, mem);
		return NULL;
	}
	linked_list_insert_tail(&ver->bufs_to_free, mem);
	return mem;
}

void _ver_dtor(ElfReaderIfc *eri)
{
	VFSElfReader *ver = (VFSElfReader *) eri;
	while (ver->bufs_to_free.count > 0)
	{
		uint8_t *mem = (uint8_t*) linked_list_remove_head(&ver->bufs_to_free);
		mf_free(ver->mf, mem);
	}
}

ElfReaderIfc *vfs_elf_reader_init(XaxVFSHandleIfc *hdl, MallocFactory *mf)
{
	VFSElfReader *ver = (VFSElfReader *) mf_malloc(mf, sizeof(VFSElfReader));
	ver->ifc.elf_read = _ver_read;
	ver->ifc.elf_read_alloc = _ver_read_alloc;
	ver->ifc.elf_dtor = _ver_dtor;
	ver->hdl = hdl;
	ver->mf = mf;
	linked_list_init(&ver->bufs_to_free, mf);
	return &ver->ifc;
}
