#include <unistd.h>
#include <sys/syscall.h>
#include "gsfree_lib.h"
#include "gsfree_syscall.h"
#include "pal_thread.h"
#include "profiler_ifc.h"

void _profiler_assert(int cond)
{
	gsfree_assert(cond);
}

int _profiler_open(const char* filename, int flags, int mode)
{
	return _gsfree_syscall(__NR_open, filename, flags, mode);
}

void _profiler_write(int fd, const void *buf, int len)
{
	_gsfree_syscall(__NR_write, fd, buf, len);
}

void _profiler_close(int fd)
{
	_gsfree_syscall(__NR_close, fd);
}

void _profiler_nanosleep(struct timespec *req)
{
	_gsfree_syscall(__NR_nanosleep, req, NULL);
}

profiler_thread_func* the_thread_func = NULL;

int profiler_thread_trampoline(void *arg)
{
	debug_announce_thread();
	return (the_thread_func)(arg);
}

int profiler_thread_create(profiler_thread_func *func, void *arg, uint32_t stack_size)
{
	_profiler_assert(the_thread_func==NULL);
	the_thread_func = func;
	int tid = pal_thread_create(profiler_thread_trampoline, arg, stack_size);
	return tid;
}

int profiler_my_id()
{
	return syscall(__NR_gettid);
}

