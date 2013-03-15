#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "LiteLib.h"
#include "zq.h"

ZQ::ZQ(MallocFactory *mf, uint32_t dbg_zutex_handle)
{
	pthread_mutex_init(&mutex, NULL);
	linked_list_init(&list, mf);
	this->dbg_zutex_handle = dbg_zutex_handle;
	this->dbg_last_waker_thread_id = -1;
	this->dbg_last_waker_match_val = -1;
	this->dbg_last_wakee_count = -1;
	this->dbg_last_wakee_first_mthread = NULL;
}

ZQ::~ZQ()
{
	pthread_mutex_destroy(&mutex);
}

#if DEBUG_ZUTEX_OWNER
void ZQ::lock(ZQLockOwnerInfo *zlo)
#else
void ZQ::lock()
#endif // DEBUG_ZUTEX_OWNER
{
	int rc;
	rc = pthread_mutex_lock(&mutex);
	assert(rc==0);
#if DEBUG_ZUTEX_OWNER
	zlo = *zlo;
#endif // DEBUG_ZUTEX_OWNER
}

void ZQ::unlock()
{
#if DEBUG_ZUTEX_OWNER
	memset(&zlo, 0, sizeof(zlo));
#endif // DEBUG_ZUTEX_OWNER
	int rc;
	rc = pthread_mutex_unlock(&mutex);
	assert(rc==0);
}

void ZQ::append(MThread *thr)
{
	linked_list_insert_tail(&list, thr);
}

void ZQ::remove(MThread *thr)
{
	// NB removes all copies, but does not preserve order.
	// (Gives O(n) instead of O(n^2), for a poor list impl.).
	LinkedListIterator lli;
	ll_start(&list, &lli);
	while (ll_has_more(&lli))
	{
		if (ll_read(&lli)==thr)
		{
			ll_remove(&list, &lli);
		}
		else
		{
			ll_advance(&lli);
		}
	}
}

MThread *ZQ::pop()
{
	return (MThread *) linked_list_remove_head(&list);
}

void ZQ::dbg_note_wake(uint32_t thread_id, uint32_t match_val, ZQ *pwq)
{
	this->dbg_last_waker_thread_id = thread_id;
	this->dbg_last_waker_match_val = match_val;
	this->dbg_last_wakee_count = pwq->list.count;

	LinkedListIterator lli;
	ll_start(&pwq->list, &lli);
	if (ll_has_more(&lli))
	{
		this->dbg_last_wakee_first_mthread = (MThread *) ll_read(&lli);
	} else {
		this->dbg_last_wakee_first_mthread = NULL;
	}
}

#if 0
uint32_t zq_len(ZQ *zq)
{
	return list.count;
}

MThread *zq_peek(ZQ *zq, uint32_t i)
{
	if (i >= list.count) { return NULL; }

	return zq->threads[0];
}
#endif

//////////////////////////////////////////////////////////////////////////////

ZQT_Entry::ZQT_Entry(uint32_t handle, MallocFactory *mf)
	: handle(handle)
{
	if (mf!=NULL)
	{
		this->zq = new ZQ(mf, handle);
	}
	else
	{
		this->zq = NULL;
	}
}

ZQT_Entry::~ZQT_Entry()
{
	delete this->zq;
}

uint32_t ZQT_Entry::hash(const void *v_a)
{
	ZQT_Entry *zqte = (ZQT_Entry *) v_a;
	return (zqte->handle>>16) ^ (zqte->handle);
}

int ZQT_Entry::cmp(const void *v_a, const void *v_b)
{
	ZQT_Entry *zqte_a = (ZQT_Entry *) v_a;
	ZQT_Entry *zqte_b = (ZQT_Entry *) v_b;
	return zqte_a->handle - zqte_b->handle;
}

//////////////////////////////////////////////////////////////////////////////

ZQTable::ZQTable(MallocFactory *mf)
	: mf(mf)
{
	hash_table_init(&ht, mf, ZQT_Entry::hash, ZQT_Entry::cmp);
	pthread_mutex_init(&tablelock, NULL);
	this->mf = mf;
}

void ZQTable::_zqt_free_one(void *v_zqt, void *v_zqte)
{
	ZQT_Entry *zqte = (ZQT_Entry *) v_zqte;
	delete zqte;
}

ZQTable::~ZQTable()
{
	pthread_mutex_lock(&tablelock);
//	hash_table_remove_every(&ht, _zqt_free_one, this);
	fprintf(stderr, "TODO ~ZQTable leaking all ZTQTEntrys to debug ptr error\n");
	pthread_mutex_unlock(&tablelock);
	pthread_mutex_destroy(&tablelock);
	memset(this, 'Y', sizeof(this));
}

ZQ *ZQTable::lookup(uint32_t *handle)
{
	pthread_mutex_lock(&tablelock);
	ZQT_Entry key((uint32_t) handle, NULL);
	ZQT_Entry *zqte = (ZQT_Entry *) hash_table_lookup(&ht, &key);
	if (zqte==NULL)
	{
		zqte = new ZQT_Entry((uint32_t) handle, mf);
		hash_table_insert(&ht, zqte);
	}
	pthread_mutex_unlock(&tablelock);

#if 0 && DEBUG_ZUTEX
	fprintf(stderr, "zqt_hash hdl 0x%08x zq 0x%08x\n",
		(uint32_t) handle, (uint32_t) (&zqte->zq));
#endif // DEBUG_ZUTEX
	return zqte->zq;
}
