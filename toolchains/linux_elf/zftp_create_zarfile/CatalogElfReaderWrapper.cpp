#include "CatalogElfReaderWrapper.h"
#include "ZFSElfReader.h"

CatalogElfReaderWrapper::CatalogElfReaderWrapper(ZFSReader* zf)
	: zf(zf),	// NB this doesn't own zf, zer does. I'm just, er, borrowing
				// a reference. Sorry.
	  zer(NULL),
	  _eo(NULL)
{
	bool is_dir_val;
	bool brc = zf->is_dir(&is_dir_val);
	lite_assert(brc);
	if (is_dir_val) { return; }	// dirs aren't Elfs.

	zer = zfs_elf_reader_init(zf);
	bool is_elf = elfobj_scan(&elfobj, zer);
	if (!is_elf) { return; }

	_eo = &elfobj;	// eo successfully constructed; be sure it gets dtored.
}

CatalogElfReaderWrapper::~CatalogElfReaderWrapper()
{
	if (_eo != NULL)
	{
		elfobj_free(_eo);	// only delete if construction was successful
	}
}

uint32_t CatalogElfReaderWrapper::get_filelen()
{
	lite_assert(_eo!=NULL);
	uint32_t filelen;
	bool rc;
	rc = zf->get_filelen(&filelen);
	lite_assert(rc);
	return filelen;
}
