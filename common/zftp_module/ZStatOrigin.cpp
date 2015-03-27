#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

#include "LiteLib.h"
#include "ZStatOrigin.h"
#include "ZFSReader.h"
#include "ZOriginFile.h"
#include "ZFSOrigin.h"	//_all_links_mtime

ZStatOrigin::ZStatOrigin(ZCache *zcache)
	: zcache(zcache),
	  file_system_view(zcache->mf, zcache->sf, false)
{
	file_system_view.insert_overlay("");
}

bool ZStatOrigin::lookup_new(const char *url)
{
	bool result;
	const char *path = _url_to_path(url);
	if (path==NULL)
	{
		return false;
	}

	ZFSReader *zfsr = file_system_view.open_path(path);

	ZNameRecord *znr;
	FreshnessSeal seal;

	if (zfsr==NULL) {
		// file not found
		seal = FreshnessSeal(1);
		hash_t zh = get_zero_hash();
		znr = new ZNameRecord(this, &zh, &seal);
	}
	else
	{
		bool rc;
		uint32_t filelen = sizeof(ZFTPUnixMetadata);

		ZFTPUnixMetadata zum;
		rc = zfsr->get_unix_metadata(&zum);
		if (!rc) { goto fail; }

		int flags = 0;

		ZOriginFile *origin_file;
		origin_file = new ZOriginFile(zcache, filelen, flags);
		origin_file->origin_load_block(0, (uint8_t*) &zum, sizeof(zum));
		origin_file->origin_finish_load();

		seal = FreshnessSeal(ZFSOrigin::_all_links_mtime(path));
		znr = new ZNameRecord(this, &origin_file->file_hash, &seal);
		delete origin_file;
	}

	zcache->insert_name(url, znr);
	delete znr;
	result = true;
	goto finally;

fail:
	result = false;

finally:
	if (zfsr!=NULL)
	{
		delete zfsr;
	}
	return result;
}

const char *ZStatOrigin::_url_to_path(const char *url)
{
	if (lite_starts_with(STAT_SCHEME, url))
	{
		return &url[lite_strlen(STAT_SCHEME)];
	}
	return NULL;
}

bool ZStatOrigin::validate(const char *url, FreshnessSeal *seal)
{
	const char *path = _url_to_path(url);
	if (path==NULL) {
		return false;
	}
	FreshnessSeal new_seal = ZFSOrigin::_all_links_mtime(path);
	return (seal->cmp(&new_seal)==0);
}
