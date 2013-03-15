#include <assert.h>
#include "MThread.h"

MThread::MThread(int thread_id, CtorType type, MallocFactory *mf)
	: thread_id(thread_id),
	  type(type),
	  tx(NULL)
{
	if (type==ACTUAL)
	{
		pthread_mutex_init(&mutex, NULL);
		linked_list_init(&zqs, mf);
	}
}

MThread::~MThread()
{
	if (type==ACTUAL)
	{
		int rc = pthread_mutex_destroy(&mutex);
		assert(rc==0);
	}
}

void MThread::wait_setup()
{
	assert(zqs.count==0);
	pthread_mutex_lock(&mutex);
}

void MThread::wait_push(ZQ *zq)
{
	linked_list_insert_tail(&zqs, zq);
}

void MThread::wait_complete(Tx *tx)
{
	assert(this->tx == NULL);
	this->tx = tx;
	assert((this->tx == NULL) == (zqs.count==0));
	pthread_mutex_unlock(&mutex);
}

void MThread::wake_setup()
{
	pthread_mutex_lock(&mutex);

	// The following asserts are intuitive, but wrong:
	// In Transaction::handle<Wake>, we hold a list of threads in
	// a private_wake_queue after we release a zq lock and before
	// we wake up the thread... That's an explicitly-allowed race,
	// during which we have committed to wake the thread, even if
	// some other wake also wakes it (and it goes to sleep and wakes
	// again; whatever). Perhaps we could get rid of the race by
	// holding the zq lock longer, but I haven't decided whether that
	// might introduce a deadlock. So instead, we simply allow the
	// race to imply the extra wake.
//	assert(tx!=NULL);
//	assert(zqs.count>0);
}

ZQ *MThread::wake_pop_zq()
{
	return (ZQ *) linked_list_remove_head(&zqs);
}

Tx *MThread::wake_get_tx()
{
	// assert(tx!=NULL);
	// When would it *not* have a tx? See comment in wake_setup().
	Tx *result = tx;
	tx = NULL;
	return result;
}

void MThread::wake_complete()
{
	assert(zqs.count == 0);
	assert(tx==NULL);
	pthread_mutex_unlock(&mutex);
}

uint32_t MThread::hash(const void *v_a)
{
	MThread *m_a = (MThread *) v_a;
	return m_a->thread_id;
}

int MThread::cmp(const void *v_a, const void *v_b)
{
	MThread *m_a = (MThread *) v_a;
	MThread *m_b = (MThread *) v_b;
	return m_a->thread_id - m_b->thread_id;
}


//////////////////////////////////////////////////////////////////////////////

ThreadTable::ThreadTable(MallocFactory *mf)
	: mf(mf)
{
	hash_table_init(&ht, mf, MThread::hash, MThread::cmp);
	pthread_mutex_init(&mutex, NULL);
}

ThreadTable::~ThreadTable()
{
	hash_table_visit_every(&ht, _free_thread, NULL);
	int rc = pthread_mutex_destroy(&mutex);
	assert(rc==0);
}

void ThreadTable::_free_thread(void *user_obj, void *a)
{
	delete ((MThread*) a);
}

MThread *ThreadTable::lookup(int thread_id)
{
	MThread *result;
	pthread_mutex_lock(&mutex);
	MThread key(thread_id, MThread::KEY, NULL);
	result = (MThread *) hash_table_lookup(&ht, &key);
	if (result==NULL)
	{
		result = new MThread(thread_id, MThread::ACTUAL, mf);
		hash_table_insert(&ht, result);
	}
	pthread_mutex_unlock(&mutex);
	return result;
}
