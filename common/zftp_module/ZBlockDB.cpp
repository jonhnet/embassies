#include <assert.h>

#include "ZBlockDB.h"
#include "ZDBCursor.h"
#include "debug.h"
#include "zlc_util.h"

ZBlockDB::ZBlockDB(ZFTPIndexConfig *zic, const char *index_dir, MallocFactory *mf)
{
	this->mf = mf;
	zdb = new ZDB(zic, index_dir, "Block.db");
}

ZBlockDB::~ZBlockDB()
{
	delete zdb;
}

void ZBlockDB::insert(ZBlockCacheRecord *block)
{
	ZDBDatum k(block->get_hash());
	ZDBDatum v((uint8_t*) block->peek_data(), ZFTP_BLOCK_SIZE, false);
	zdb->insert(&k, &v);
}

ZBlockCacheRecord *ZBlockDB::lookup(const hash_t *block_hash)
{
	ZDBDatum k(block_hash);
	ZDBDatum *v = zdb->lookup(&k);
	if (v==NULL)
	{
		return NULL;
	}
	ZBlockCacheRecord *zbr = new ZBlockCacheRecord(mf, block_hash, v);
	return zbr;
}

void ZBlockDB::debug_dump()
{
	ZDBCursor *c = zdb->create_cursor();
	fprintf(stderr, "Dumping %s\n", zdb->get_name());
	while (true)
	{
		ZDBDatum *k, *v;
		bool rc = c->get(&k, &v);
		if (!rc)
		{
			break;
		}
		hash_t hash_key = k->read_hash();

		char hashbuf[sizeof(hash_t)*2+1];
		debug_hash_to_str(hashbuf, &hash_key);
		fprintf(stderr, "block at '%s'\n", hashbuf);
		delete k;
		delete v;
	}
	delete c;
}
