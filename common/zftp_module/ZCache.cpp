#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif // _WIN32
#include <string.h>

#include "LiteLib.h"
#include "standard_malloc_factory.h"
#include "ZCache.h"
#include "ZFileServer.h"
#include "ZFileClient.h"
#include "ZFSOrigin.h"
#include "ZStatOrigin.h"
#include "ZRFOrigin.h"
#include "ZBlockDB.h"
#include "ZLCArgs.h"
#include "ZAuthoritativeOrigin.h"
#include "SyncFactory.h"

ZCache::ZCache(ZLCArgs *zargs, MallocFactory *mf, SyncFactory *sf, ZLCEmit *ze, SendBufferFactory *sbf, ZCompressionIfc* zcompression)
{
	this->mf = mf;
	this->sf = sf;
	this->zcompression = zcompression;
	this->ze = ze;
	this->sbf = sbf;
	this->pmi = pmi;

	uint8_t *zeros = (uint8_t*) alloca(ZFTP_BLOCK_SIZE);
	memset(zeros, 0, ZFTP_BLOCK_SIZE);
	zhash(zeros, ZFTP_BLOCK_SIZE, &hash_of_ZFTP_BLOCK_SIZE_zeros);

	hash_table_init(&file_ht, mf, ZCachedFile::__hash__, ZCachedFile::__cmp__);
	hash_table_init(&name_ht, mf, ZCachedName::__hash__, ZCachedName::__cmp__);

	zfile_server = NULL;
	zfile_client = NULL;

	if (zargs->index_dir != NULL)
	{
#if ZLC_USE_PERSISTENT_STORAGE
		zic = new ZFTPIndexConfig();
		znameDB = new ZNameDB(this, zic, zargs->index_dir);
		zfileDB = new ZFileDB(zic, zargs->index_dir);
		zblockDB = new ZBlockDB(zic, zargs->index_dir, mf);
#else
		lite_assert(false);	// no DB support
#endif // ZLC_USE_PERSISTENT_STORAGE
	}
	else
	{
		zic = NULL;
		znameDB = NULL;
		zfileDB = NULL;
		zblockDB = NULL;
	}

	block_cache = new ZBlockCache(this, mf, sf);

	linked_list_init(&origins, mf);
	hash_table_init(&name_to_origin, mf, ZOrigin::__hash__, ZOrigin::__cmp__);

	if (zargs->origin_filesystem)
	{
#if ZLC_USE_ORIGINS
		_add_origin(new ZFSOrigin(this));
		_add_origin(new ZStatOrigin(this));
#else
		lite_assert(false);	// no origins available
#endif // ZLC_USE_ORIGINS
	}
	if (zargs->origin_reference)
	{
#if ZLC_USE_ORIGINS
		_add_origin(new ZRFOrigin(this));
#else
		lite_assert(false);	// no origins available
#endif // ZLC_USE_ORIGINS
	}
	if (origins.count>0)
	{
#if ZLC_USE_ORIGINS
		_add_origin(new ZAuthoritativeOrigin(this));
#else
		lite_assert(false);	// no origins available
#endif // ZLC_USE_ORIGINS
	}

	this->configured = sf->new_event(false);

	zero_hash = get_zero_hash();
}

void ZCache::_add_origin(ZOrigin *origin)
{
	linked_list_insert_tail(&origins, origin);
	hash_table_insert(&name_to_origin, origin);
}

void ZCache::_delete_cached_file(void *v_this, void *datum)
{
	ZCachedFile *zcf = (ZCachedFile *) datum;
	delete zcf;
}

void ZCache::_delete_name_record(void *v_this, void *datum)
{
	ZNameRecord *znr = (ZNameRecord *) datum;
	delete znr;
}

ZCache::~ZCache()
{
	hash_table_remove_every(&file_ht, _delete_cached_file, NULL);
		// TODO when we have eviction, change to evict(0)
	lite_assert(file_ht.count==0);

	hash_table_remove_every(&name_ht, _delete_name_record, NULL);
	lite_assert(name_ht.count==0);

	delete block_cache;

#if ZLC_USE_PERSISTENT_STORAGE
	delete znameDB;
	delete zfileDB;
	delete zblockDB;
	delete zic;
#endif // ZLC_USE_PERSISTENT_STORAGE
}

void ZCache::configure(ZFileServer *_zfile_server, ZFileClient *_zfile_client)
{
	this->zfile_server = _zfile_server;
	this->zfile_client = _zfile_client;
	configured->signal();
}

ZCachedFile *ZCache::lookup_hash(const hash_t *file_hash)
{
	ZCachedFile key(file_hash);
	ZCachedFile *zcf = (ZCachedFile *) hash_table_lookup(&file_ht, &key);
	if (zcf==NULL)
	{
		zcf = new ZCachedFile(this, file_hash);
		hash_table_insert(&file_ht, zcf);
	}
	return zcf;
}

ZCachedName *ZCache::lookup_url(const char *url_hint)
{
	ZCachedName key(this, url_hint, mf);
	ZCachedName *zcn = (ZCachedName *) hash_table_lookup(&name_ht, &key);
	if (zcn==NULL)
	{
		zcn = new ZCachedName(this, url_hint, mf);
		hash_table_insert(&name_ht, zcn);
	}
	return zcn;
}

void ZCache::probe_origins(const char *url)
{
	LinkedListLink *link;
	for (link = origins.head;
		link!=NULL;
		link = link->next)
	{
		ZOrigin *origin = (ZOrigin*) link->datum;
		bool found = origin->lookup_new(url);
		if (found)
		{
			break;
		}
	}
}

bool ZCache::probe_origins_for_hash(const char *url, hash_t *hash)
{
	ZCachedName *zcn = lookup_url(url);

	hash_t *known_hash;
	known_hash = zcn->known_validated_hash();
	if (known_hash!=NULL && cmp_hash(known_hash, hash)!=0)
	{
		return false;
	}

	probe_origins(url);

	known_hash = zcn->known_validated_hash();
	lite_assert(known_hash!=NULL); // file-not-found should become zero hash
		// (ugh, unless we're not the origin?)
	if (cmp_hash(known_hash, hash)!=0)
	{
		return false;
	}
	return true;
}

void ZCache::debug_dump_db()
{
#if ZLC_USE_PERSISTENT_STORAGE
	znameDB->debug_dump();
	zfileDB->debug_dump();
	zblockDB->debug_dump();
#endif // ZLC_USE_PERSISTENT_STORAGE
}

bool ZCache::insert_block(ZBlockCacheRecord *block)
{
	return block_cache->insert(block);
}

ZOrigin *ZCache::lookup_origin(const char *class_name)
{
	ZOriginKey key(class_name);
	ZOrigin *origin = (ZOrigin*) hash_table_lookup(&name_to_origin, &key);
	return origin;
}

void ZCache::insert_name(const char *url, ZNameRecord *znr)
{
	ZCachedName *zcn = lookup_url(url);
	zcn->install_from_origin(znr);
}

void ZCache::store_name(const char *url, ZNameRecord *znr)
{
	if (znameDB!=NULL)
	{
#if ZLC_USE_PERSISTENT_STORAGE
		znameDB->insert(url, znr);
#else
		lite_assert(false);
#endif // ZLC_USE_PERSISTENT_STORAGE
	}
}

ZNameRecord *ZCache::retrieve_name(const char *url)
{
	if (znameDB!=NULL)
	{
#if ZLC_USE_PERSISTENT_STORAGE
		return znameDB->lookup(url);
#else
		lite_assert(false);
#endif // ZLC_USE_PERSISTENT_STORAGE
	}
	return NULL;
}

void ZCache::store_file(ZCachedFile *zcf)
{
	if (zfileDB!=NULL)
	{
#if ZLC_USE_PERSISTENT_STORAGE
		zfileDB->insert(zcf);
#else
		lite_assert(false);
#endif // ZLC_USE_PERSISTENT_STORAGE
	}
}

ZCachedFile *ZCache::retrieve_file(hash_t *file_hash)
{
	if (zfileDB!=NULL)
	{
#if ZLC_USE_PERSISTENT_STORAGE
		return zfileDB->lookup(this, file_hash);
#else
		lite_assert(false);
#endif // ZLC_USE_PERSISTENT_STORAGE
	}
	return NULL;
}

void ZCache::store_block(ZBlockCacheRecord *zbr)
{
	if (zblockDB!=NULL)
	{
#if ZLC_USE_PERSISTENT_STORAGE
		zblockDB->insert(zbr);
#else
		lite_assert(false);
#endif // ZLC_USE_PERSISTENT_STORAGE
	}
}

ZBlockCacheRecord *ZCache::retrieve_block(const hash_t *hash)
{
	if (zblockDB!=NULL)
	{
#if ZLC_USE_PERSISTENT_STORAGE
		return zblockDB->lookup(hash);
#else
		lite_assert(false);
#endif // ZLC_USE_PERSISTENT_STORAGE
	}
	return NULL;
}

void ZCache::configure_perf_measure_ifc(PerfMeasureIfc* pmi)
{
	lite_assert(this->pmi == NULL);
	this->pmi = pmi;
}
