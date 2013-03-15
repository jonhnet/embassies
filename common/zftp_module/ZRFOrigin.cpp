#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

#include "LiteLib.h"
#include "ZRFOrigin.h"
#include "ZCachedFile.h"
#include "reference_file.h"
#include "zftp_util.h"
#include "ZOriginFile.h"

ZRFOrigin::ZRFOrigin(ZCache *zcache)
{
	this->zcache = zcache;
}

bool ZRFOrigin::lookup_new(const char *url)
{
	int selector = _url_to_selector(url);
	if (selector < 0)
	{
		return false;
	}

	ReferenceFile *rf = reference_file_init(zcache->mf, selector);

	ZOriginFile *origin_file =
		new ZOriginFile(zcache, rf->len, 0);

	uint32_t offset;
	uint32_t blk;
	for (offset=0, blk=0;
		offset < rf->len;
		offset+=ZFTP_BLOCK_SIZE,
		blk+=1)
	{
		int bytes_remaining = min(ZFTP_BLOCK_SIZE, (int) (rf->len-offset));
		origin_file->origin_load_block(blk, &rf->bytes[offset], bytes_remaining);
	}

	origin_file->origin_finish_load();

	FreshnessSeal seal(1);
	ZNameRecord *znr;
	znr = new ZNameRecord(this, &origin_file->file_hash, &seal);
	delete origin_file;

	zcache->insert_name(url, znr);
	delete znr;
	return true;
}

int ZRFOrigin::_url_to_selector(const char *url)
{
	if (lite_starts_with(REFERENCE_FILE_SCHEME, url))
	{
		return atoi(&url[lite_strlen(REFERENCE_FILE_SCHEME)]);
	}
	return -1;
}

bool ZRFOrigin::validate(const char *url, FreshnessSeal *seal)
{
	// reference files never go out of style.
	return true;
}
