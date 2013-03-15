#include "sched.h"
#include "pal_thread.h"
#include "gsfree_lib.h"

int pal_thread_create(pal_thread_func *func, void *arg, uint32_t stack_size)
{
	//void *stack = mf_malloc(mf, stack_size);
		// use a page-guarded stack. Malloc'd stacks stink because
		// when they stomp, it's really confusing.
	void *stack = debug_alloc_protected_memory(stack_size);
	int flags = CLONE_VM| CLONE_FS| CLONE_FILES| CLONE_SIGHAND| CLONE_THREAD| CLONE_SYSVSEM;
	int rc = clone(func, stack+stack_size-4, flags, arg);
	gsfree_assert(rc>0);
	return rc;
}
