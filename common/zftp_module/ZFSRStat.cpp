#define _GNU_SOURCE 1
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ZFSRStat.h"
#include "debug.h"
#include "xax_network_utils.h"

ZFSRStat::ZFSRStat()
{
	backing = NULL;
}

bool ZFSRStat::open(ZFSReader *backing)
{
	this->backing = backing;
	
	return true;
}

ZFSRStat::~ZFSRStat()
{
	if (backing!=NULL)
	{
		delete backing;
	}
}

bool ZFSRStat::is_dir(bool *isdir_out)
{
	*isdir_out = false;
	return true;
}

uint32_t ZFSRStat::get_data(uint32_t offset, uint8_t *buf, uint32_t buf_len)
{
	assert(offset==0);
	assert(buf_len==sizeof(ZFTPUnixMetadata));
	bool rc;
	rc = backing->get_unix_metadata((ZFTPUnixMetadata*) buf);
	assert(rc);
	return sizeof(ZFTPUnixMetadata);
}


bool ZFSRStat::is_cacheable()
{
	return true;
}

bool ZFSRStat::get_filelen(uint32_t *filelen_out)
{
	*filelen_out = sizeof(ZFTPUnixMetadata);
	return true;
}

bool ZFSRStat::get_mtime(uint32_t *mtime_out)
{
	return false;
}

bool ZFSRStat::get_unix_metadata(ZFTPUnixMetadata *out_zum)
{
	return false;
}
