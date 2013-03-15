#define _GNU_SOURCE 1
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ZFSReference.h"
#include "debug.h"

ZFSReference::ZFSReference(MallocFactory *mf, uint32_t pattern_selector)
{
	reference_file = reference_file_init(mf, pattern_selector);
}

ZFSReference::~ZFSReference()
{
	reference_file_free(reference_file);
}

bool ZFSReference::get_filelen(uint32_t *filelen_out)
{
	*filelen_out = reference_file->len;
	return true;
}

bool ZFSReference::is_dir(bool *isdir_out)
{
	*isdir_out = false;
	return true;
}

uint32_t ZFSReference::get_data(uint32_t offset, uint8_t *buf, uint32_t buf_len)
{
	uint32_t len = buf_len;
	if (offset + len > reference_file->len)
	{
		len = reference_file->len - offset;
		assert(len > 0);
	}
	memcpy(buf, reference_file->bytes, len);
	return len;
}

bool ZFSReference::get_mtime(uint32_t *mtime_out)
{
	*mtime_out = 0;
	return true;
}

bool ZFSReference::get_unix_metadata(ZFTPUnixMetadata *out_zum)
{
	return false;
}

bool ZFSReference::is_cacheable()
{
	return false;
}
