#include "zftp_protocol.h"
#include "ZBlockCache.h"
#include "zlc_util.h"
#include "ZCache.h"

ZBlockCache::ZBlockCache(ZCache *zcache, MallocFactory *mf, SyncFactory *sf)
{
	this->zcache = zcache;
	hash_table_init(&ht, mf,
		ZBlockCacheRecord::__hash__, ZBlockCacheRecord::__cmp__);

	mutex = sf->new_mutex(false);
}

ZBlockCache::~ZBlockCache()
{
	hash_table_remove_every(&ht, _free_block, this);
}

void ZBlockCache::_free_block(void *v_this, void *v_zbcr)
{
	//ZBlockCache *this = (ZBlockCache *)v_this;
	ZBlockCacheRecord *zbcr = (ZBlockCacheRecord *) v_zbcr;
	delete zbcr;
}

ZBlockCacheRecord *ZBlockCache::lookup(const hash_t *hash)
{
	mutex->lock();
	ZBlockCacheRecord *result = _lookup_nolock(hash);
	mutex->unlock();
	return result;
}

ZBlockCacheRecord *ZBlockCache::_lookup_nolock(const hash_t *hash)
{
	ZBlockCacheRecord key(hash);
	ZBlockCacheRecord *result = (ZBlockCacheRecord *)
		hash_table_lookup(&ht, &key);
	if (result==NULL)
	{
		
		result = zcache->retrieve_block(hash);
		if (result!=NULL)
		{
			hash_table_insert(&ht, result);
			ZBlockCacheRecord *result_again = (ZBlockCacheRecord *)
				hash_table_lookup(&ht, &key);
			lite_assert(result==result_again);	// we should get back the one we put in.
		}
	}
	return result;
}

bool ZBlockCache::insert(ZBlockCacheRecord *block)
{
	bool result;
	mutex->lock();

	if (_lookup_nolock(block->get_hash())==NULL)
	{
		hash_table_insert(&ht, block);
		zcache->store_block(block);
		result = true;
	}
	else
	{
		// already have it.
		delete block;
		result = false;
	}

	mutex->unlock();
	return result;
}
