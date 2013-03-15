#include <malloc.h>
#include "ZFSRDir_Union.h"

ZFSRDir_Union::ZFSRDir_Union(MallocFactory *mf, SyncFactory *sf, ZFSRDir *a, ZFSRDir *b)
	: mf(mf),
	  a(a),
	  b(b),
	  tree(sf)
{
}

bool ZFSRDir_Union::fetch_stat()
{
	a->get_unix_metadata(&this->zum);
	return true;
}

void ZFSRDir_Union::ingest(HandyDirRec *hdr)
{
	if (tree.lookup(hdr))
	{
		delete hdr;
	}
	else
	{
		tree.insert(hdr);
	}
}

bool ZFSRDir_Union::scan_one_dir(ZFSRDir *d)
{
	// factoring fail. We're de-encoding other dirs to pull them into
	// this dir. Silly.
	bool rc;
	uint32_t offset = 0;
	ZFTPDirectoryRecord drec;

	uint32_t filelen;
	rc = d->get_filelen(&filelen);
	lite_assert(rc);
	while (offset < filelen)
	{
		uint32_t got = d->get_data(offset, (uint8_t*) &drec, sizeof(drec));
		lite_assert(got==sizeof(drec));
		ZFTPDirectoryRecord *dcopy = (ZFTPDirectoryRecord *)
			malloc(drec.reclen);
		got = d->get_data(offset, (uint8_t*) dcopy, drec.reclen);
		lite_assert(got==drec.reclen);
		HandyDirRec *hdr = new HandyDirRec(
			mf, dcopy, ZFTPDirectoryRecord_filename(dcopy));
		free(dcopy);
		ingest(hdr);
		offset += drec.reclen;
	}

	return true;
}

void ZFSRDir_Union::squirt_tree()
{
	while (tree.size() > 0)
	{
		HandyDirRec *hdr = tree.findMin();
		tree.remove(hdr);
		scan_add_one(hdr);
	}
}

bool ZFSRDir_Union::scan()
{
	bool rc;
	rc = scan_one_dir(a);
	lite_assert(rc);
	rc = scan_one_dir(b);
	lite_assert(rc);
	squirt_tree();
	return true;
}


