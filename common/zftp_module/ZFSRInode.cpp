#define _GNU_SOURCE 1
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ZFSRInode.h"
#include "zftp_unix_metadata_format.h"
#include "xax_network_utils.h"

ZFSRInode::ZFSRInode()
{
	filelen = 0;
}

ZFSRInode::~ZFSRInode()
{}

void ZFSRInode::fill_from_stat(struct stat *statbuf)
{
	mtime = (uint32_t) statbuf->st_mtime;
	zum.dev = z_htonw(statbuf->st_dev);
	zum.ino = z_htonw(statbuf->st_ino);
	zum.mtime = z_htonw(statbuf->st_mtime);
}

bool ZFSRInode::get_filelen(uint32_t *filelen_out)
{
	*filelen_out = filelen;
	return true;
}

bool ZFSRInode::get_mtime(uint32_t *mtime_out)
{
	*mtime_out = mtime;
	return true;
}

bool ZFSRInode::get_unix_metadata(ZFTPUnixMetadata *out_zum)
{
	*out_zum = zum;
	return true;
}


