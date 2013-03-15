#include <assert.h>
#include <malloc.h>

#include "ZNameDB.h"
#include "ZDBCursor.h"
#include "debug.h"
#include "zlc_util.h"

ZNameDB::ZNameDB(ZCache *zcache, ZFTPIndexConfig *zic, const char *index_dir)
{
	this->zcache = zcache;
	zdb = new ZDB(zic, index_dir, "Name.db");
}

ZNameDB::~ZNameDB()
{
	delete zdb;
}

void ZNameDB::insert(const char *url, ZNameRecord *znr)
{
	ZDBDatum k(url);
	ZDBDatum *v = znr->marshal();
	zdb->insert(&k, v);
}

ZNameRecord *ZNameDB::lookup(const char *url)
{
	ZDBDatum k(url);
	ZDBDatum *v = zdb->lookup(&k);
	if (v==NULL)
	{
		return NULL;
	}
	ZNameRecord *znr = new ZNameRecord(zcache, v);
	delete v;
	return znr;
}

void ZNameDB::debug_dump()
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
		const char *url_key = k->read_str();
		ZNameRecord *znr = new ZNameRecord(zcache, v);
		char hashbuf[sizeof(hash_t)*2+1];
		debug_hash_to_str(hashbuf, znr->get_file_hash());
		fprintf(stderr, "'%s' => %s\n", url_key, hashbuf);
		delete k;
		delete v;
	}
	delete c;
}
