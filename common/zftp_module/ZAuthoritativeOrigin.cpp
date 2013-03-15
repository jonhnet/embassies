#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

#include "LiteLib.h"
#include "ZAuthoritativeOrigin.h"
#include "ZOriginFile.h"

ZAuthoritativeOrigin::ZAuthoritativeOrigin(ZCache *zcache)
{
	this->zcache = zcache;
}

bool ZAuthoritativeOrigin::lookup_new(const char *url)
{
	FreshnessSeal seal;
	hash_t zh = get_zero_hash();
	ZNameRecord *znr = new ZNameRecord(this, &zh, &seal);
	zcache->insert_name(url, znr);
	delete znr;
	return true;
}

bool ZAuthoritativeOrigin::validate(const char *url, FreshnessSeal *seal)
{
	assert(false);
	return false;
}
