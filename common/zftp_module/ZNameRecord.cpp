#include <assert.h>
#include <malloc.h>

#include "ZNameRecord.h"
#include "ZCache.h"

ZNameRecord::ZNameRecord(ZOrigin *origin, hash_t *file_hash, FreshnessSeal *seal)
{
	this->origin = origin;
	this->file_hash = *file_hash;
	this->seal = *seal;
}

ZNameRecord::ZNameRecord(ZNameRecord *znr)
{
	this->origin = znr->origin;
	this->file_hash = znr->file_hash;
	this->seal = znr->seal;
}

ZNameRecord::~ZNameRecord()
{
}

#if ZLC_USE_PERSISTENT_STORAGE
ZNameRecord::ZNameRecord(ZCache *zcache, ZDBDatum *zd)
{
	this->origin = zcache->lookup_origin(zd->read_str());
	zd->read_buf(&file_hash, sizeof(file_hash));
	zd->read_buf(&seal, sizeof(seal));
}

void ZNameRecord::_marshal(ZDBDatum *zd)
{
	zd->write_str(origin->get_origin_name());
	zd->write_buf(&file_hash, sizeof(file_hash));
	zd->write_buf(&seal, sizeof(seal));
}

ZDBDatum *ZNameRecord::marshal()
{
	ZDBDatum count;
	_marshal(&count);
	ZDBDatum *result = count.alloc();
	_marshal(result);
	return result;
}
#endif

bool ZNameRecord::check_freshness(const char *url)
{
	return origin->validate(url, &seal);
}
