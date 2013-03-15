#include <assert.h>
#include <pthread.h>
#include "ThreadFactory_Pthreads.h"

ThreadFactory_Pthreads::ThreadFactory_Pthreads()
{
}

void ThreadFactory_Pthreads::create_thread(cheesythread_f *func, void *arg, uint32_t stack_size)
{
	assert(stack_size < (2<<20));
		// pthreads on linux default is 2MB, so let's just not sweat it.

	typedef void *(pthread_start_routine)(void *);
	pthread_start_routine *pthread_func = (pthread_start_routine*) func;
	pthread_t thread;
	pthread_create(&thread, NULL, pthread_func, arg);
}
