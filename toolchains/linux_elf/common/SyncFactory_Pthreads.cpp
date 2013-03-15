#include <assert.h>
#include <errno.h>
#include <sys/time.h>
#include "SyncFactory_Pthreads.h"

SyncFactoryMutex_Pthreads::SyncFactoryMutex_Pthreads(bool recursive)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	if (recursive)
	{
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	}
	pthread_mutex_init(&mutex, &attr);
	pthread_mutexattr_destroy(&attr);
}

SyncFactoryMutex_Pthreads::~SyncFactoryMutex_Pthreads()
{
	pthread_mutex_destroy(&mutex);
}

void SyncFactoryMutex_Pthreads::lock()
{
	pthread_mutex_lock(&mutex);
}

void SyncFactoryMutex_Pthreads::unlock()
{
	pthread_mutex_unlock(&mutex);
}


SyncFactoryEvent_Pthreads::SyncFactoryEvent_Pthreads(bool auto_reset)
{
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	this->auto_reset = auto_reset;
	this->event_is_set = false;
}

SyncFactoryEvent_Pthreads::~SyncFactoryEvent_Pthreads()
{
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
}

void SyncFactoryEvent_Pthreads::wait()
{
	pthread_mutex_lock(&mutex);
	while (!event_is_set)
	{
		pthread_cond_wait(&cond, &mutex);
	}
	if (auto_reset)
	{
		event_is_set = false;
	}
	pthread_mutex_unlock(&mutex);
}

bool SyncFactoryEvent_Pthreads::wait(int timeout_ms)
{
	struct timeval tv_start;
	int rc = gettimeofday(&tv_start, NULL);
	assert(rc==0);

	pthread_mutex_lock(&mutex);
	while (!event_is_set)
	{
		struct timeval tv;
		rc = gettimeofday(&tv, NULL);
		assert(rc==0);
		int elapsed_ms =
			(tv.tv_usec - tv_start.tv_usec)/1000 + (tv.tv_sec - tv_start.tv_sec)*1000;
		int remaining_ms = timeout_ms - elapsed_ms;
		if (remaining_ms < 0)
		{
			break;
		}

		struct timespec timeout;
		int to_sec = timeout_ms/1000;
		int to_msec = timeout_ms - to_sec*1000;
		int carry_sec = 0;
		timeout.tv_nsec = tv_start.tv_usec*1000 + to_msec*1000000;
		if (timeout.tv_nsec >= 1000000000)
		{
			timeout.tv_nsec -= 1000000000;
			carry_sec = 1;
		}
		assert(timeout.tv_nsec < 1000000000);
		timeout.tv_sec = tv_start.tv_sec + to_sec + carry_sec;

		rc = pthread_cond_timedwait(&cond, &mutex, &timeout);
		if (rc!=ETIMEDOUT)
		{
			if (auto_reset)
			{
				event_is_set = false;
			}
			break;
		}
		// if timed out, loop back around and check the time.
	}

	bool succeeded = event_is_set;
	pthread_mutex_unlock(&mutex);
	return succeeded;
}

void SyncFactoryEvent_Pthreads::signal()
{
	pthread_mutex_lock(&mutex);
	event_is_set = true;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

void SyncFactoryEvent_Pthreads::reset()
{
	pthread_mutex_lock(&mutex);
	event_is_set = false;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

SyncFactory_Pthreads::SyncFactory_Pthreads()
{
}

SyncFactoryMutex *SyncFactory_Pthreads::new_mutex(bool recursive)
{
	return new SyncFactoryMutex_Pthreads(recursive);
}

SyncFactoryEvent *SyncFactory_Pthreads::new_event(bool auto_reset)
{
	return new SyncFactoryEvent_Pthreads(auto_reset);
}


