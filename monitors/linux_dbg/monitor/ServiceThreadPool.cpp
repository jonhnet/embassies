#include <malloc.h>
#include <pthread.h>

#include "ServiceThreadPool.h"
#include "Monitor.h"

ServiceThreadPool::ServiceThreadPool(Monitor *m)
{
	this->m = m;
	pthread_mutex_init(&mutex, NULL);
	idle_threads = 0;
	pool_threads = 0;
}

void ServiceThreadPool::grow()
{
	int dbg_num_pool_threads;
	pthread_mutex_lock(&mutex);
	idle_threads += 1;
	pool_threads += 1;
	dbg_num_pool_threads = pool_threads;
	pthread_mutex_unlock(&mutex);

	m->start_new_thread();

	fprintf(stderr, "service_thread_pool now %d\n", dbg_num_pool_threads);
}

void ServiceThreadPool::departing()
{
	bool grow_pool;
	pthread_mutex_lock(&mutex);
	idle_threads -= 1;
	grow_pool = (idle_threads == 0);
	pthread_mutex_unlock(&mutex);
	
	if (grow_pool)
	{
		grow();
	}
}

void ServiceThreadPool::returning()
{
	pthread_mutex_lock(&mutex);
	idle_threads += 1;
	pthread_mutex_unlock(&mutex);
}
