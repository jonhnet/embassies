#include <unistd.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include "LiteLib.h"

void _profiler_assert(bool cond)
{
	lite_assert(cond);
}

int _profiler_open(const char* filename, int flags, int mode)
{
	return open(filename, flags, mode);
}

void _profiler_write(int fd, const void *buf, int len)
{
	write(fd, buf, len);
}

void _profiler_close(int fd)
{
	close(fd);
}

void _profiler_nanosleep(struct timespec *req)
{
	nanosleep(req, NULL);
}

typedef int (profiler_thread_func)(void *arg);
typedef void *(pthread_thread_func)(void *arg);

int profiler_thread_create(profiler_thread_func *func, void *arg, uint32_t stack_size)
{
	pthread_t* pt = (pthread_t*) malloc(sizeof(pthread_t));
	pthread_create(pt, NULL, (pthread_thread_func*) func, arg);
	return 0;	// Uh-oh, we don't have a way to mask off the dump_thread. Yet.
}

int profiler_my_id()
{
	_profiler_assert(sizeof(pthread_t)==sizeof(int));
	return (int) pthread_self();
}
