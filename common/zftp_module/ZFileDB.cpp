#include <assert.h>

#include "ZFileDB.h"
#include "ZDBCursor.h"
#include "debug.h"
#include "zlc_util.h"

ZFileDB::ZFileDB(ZFTPIndexConfig *zic, const char *index_dir)
{
	zdb = new ZDB(zic, index_dir, "File.db");
}

ZFileDB::~ZFileDB()
{
	delete zdb;
}

void ZFileDB::insert(ZCachedFile *zcf)
{
	ZDBDatum k(&zcf->file_hash);
	ZDBDatum count;
	zcf->marshal(&count);
	ZDBDatum *v = count.alloc();
	zcf->marshal(v);
	zdb->insert(&k, v);
}

ZCachedFile *ZFileDB::lookup(ZCache *zcache, hash_t *file_hash)
{
	ZDBDatum k(file_hash);
	ZDBDatum *v = zdb->lookup(&k);
	if (v==NULL)
	{
		return NULL;
	}
	ZCachedFile *zcf = new ZCachedFile(zcache, v);
	return zcf;
}

void ZFileDB::debug_dump()
{
	ZDBCursor *c = zdb->create_cursor();
	fprintf(stderr, "Dumping %s [%d]\n", zdb->get_name(), zdb->get_nkeys());
	while (true)
	{
		ZDBDatum *k, *v;
		bool rc = c->get(&k, &v);
		if (!rc)
		{
			break;
		}
		hash_t hash_key = k->read_hash();

		ZCachedFile zcf(NULL, v);

		char hashbuf[sizeof(hash_t)*2+1];
		debug_hash_to_str(hashbuf, &hash_key);
		fprintf(stderr, "'%s' => ZCF(filelen %d, tree size %d)\n",
			hashbuf, zcf.file_len, zcf.tree_size);
		delete k;
		delete v;
	}
	delete c;
}
