#include <base/printf.h>
#include "abort.h"
#include "ThreadTable.h"
#include "common/Thread_id.h"
#include "LiteLib.h"

struct ThreadRecord {
	Thread_id id;
	ZoogThread *thread_obj;

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);
};

uint32_t ThreadRecord::hash(const void *v_a)
{
	ThreadRecord *rec = ((ThreadRecord *) v_a);
	return hash_buf(&rec->id, sizeof(rec->id));
}

int ThreadRecord::cmp(const void *v_a, const void *v_b)
{
	ThreadRecord *rec_a = ((ThreadRecord *) v_a);
	ThreadRecord *rec_b = ((ThreadRecord *) v_b);
	return cmp_bufs(
		&rec_a->id, sizeof(rec_a->id),
		&rec_b->id, sizeof(rec_b->id));
}

ThreadTable::ThreadTable()
{
	hash_table_init(&ht, standard_malloc_factory_init(), ThreadRecord::hash, ThreadRecord::cmp);
}

void ThreadTable::insert_me(ZoogThread *t)
{
	ThreadRecord *rec = new ThreadRecord;
	rec->id = Thread_id::get_my();
	rec->thread_obj = t;
	PDBG("%08x\n", rec->hash(rec));
	hash_table_insert(&ht, rec);
}

ZoogThread *ThreadTable::lookup_me()
{
	ThreadRecord key = { Thread_id::get_my() };
	ThreadRecord *rec = (ThreadRecord*) hash_table_lookup(&ht, &key);
	if (rec==NULL)
	{
		PDBG("Rats; Thread_id %08x doesn't map\n", key.hash(&key));
		abort("lookup_me() failed");
	}
	return rec->thread_obj;
}

ZoogThread *ThreadTable::remove_me()
{
	ThreadRecord key = { Thread_id::get_my() };
	PDBG("%08x\n", key.hash(&key));
	ThreadRecord *rec = (ThreadRecord*) hash_table_lookup(&ht, &key);
	hash_table_remove(&ht, rec);
	ZoogThread *t = rec->thread_obj;
	delete rec;
	return t;
}

class ThreadEnumerate {
public:
	ZoogThread **ary;
	int idx;
};

static void enumerate_func(void *user_obj, void *a)
{
	ThreadEnumerate *te = (ThreadEnumerate *) user_obj;
	ThreadRecord *tr = (ThreadRecord *) a;
	te->ary[te->idx++] = tr->thread_obj;
}

void ThreadTable::enumerate(ZoogThread **ary, int capacity)
{
	lite_assert(ht.count == capacity);
	ThreadEnumerate te;
	te.ary = ary;
	te.idx = 0;
	hash_table_visit_every(&ht, enumerate_func, &te);
	lite_assert(te.idx==ht.count);
}
