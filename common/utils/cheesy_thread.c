#include "cheesy_thread.h"
#include "xax_util.h"
#include "LiteLib.h"
#include "pal_abi/pal_extensions.h"

typedef struct {
	void *set_gs_base;
	ZoogDispatchTable_v1 *zdt;
	cheesythread_f *user_func;
	void *user_arg;
} cheesy_thread_argblock;

void cheesy_thread_start(void *cheesy_arg)
{
	__asm__(
		"movl	%%ebp,%%eax;"
		"movl	$0,0(%%eax);"
		"movl	$0,4(%%eax);"
	:
	:
	: "%eax"
	);
	cheesy_thread_argblock *argblock = (cheesy_thread_argblock *) cheesy_arg;
	(argblock->zdt->zoog_x86_set_segments)(0, (uint32_t) argblock->set_gs_base);

	debug_announce_thread_f *announce_thread =
		argblock->zdt->zoog_lookup_extension("debug_announce_thread");
	if (announce_thread!=NULL)
	{
		(*announce_thread)();
		// perhaps there's a way to tell gdb about multiple threads
		// in a core file?
	}

	(argblock->user_func)(argblock->user_arg);

	(argblock->zdt->zoog_thread_exit)();
}

void cheesy_thread_create(ZoogDispatchTable_v1 *zdt, cheesythread_f *user_func, void *user_arg, uint32_t stack_size)
{
	// wow, we've been burned a lot by too-small threads.
	// Let's use zdt memory allocation (which, when possible, provides
	// a guard page -- e.g. zoog_port_pal compiled with GUARD=1).
	// I exposed stack_size so threads could just take a little responsibility
	// for their requirements.
	uint32_t cheesy_argblock_size = sizeof(cheesy_thread_argblock);

	// I doubt we actually need TLS space here, since I think cheesy_threads
	// are for zone0 code, which shouldn't ever touch TLS anyway.
	uint32_t tls_size = 2848;
	uint32_t tls_positive_side = 1136;
		// Got by inspecting a pthreads-linked program [2010.11.15].
		// I Really have no idea how to know how much space to leave.
	uint32_t thread_size = stack_size + cheesy_argblock_size + tls_size;
	//void *thread_space = mf->c_malloc(mf, thread_size);
	void *thread_space = (zdt->zoog_allocate_memory)(thread_size);
	lite_assert(thread_space != NULL);

	// try to catch invalid assumptions about what's in the thread_space.
	lite_memset(thread_space, 0xcc, thread_size);

	// tcb should point to itself so it can be loaded without "reading" gs
	void **tcb = (void**) (thread_space + thread_size - tls_positive_side);
	*tcb = (void*) tcb;

	cheesy_thread_argblock *argblock =
		(cheesy_thread_argblock*)(thread_space+stack_size);
	argblock->set_gs_base = (void*) tcb;
	argblock->zdt = zdt;
	argblock->user_func = user_func;
	argblock->user_arg = user_arg;

	void *child_stack = thread_space + stack_size;
	child_stack -= sizeof(uint32_t);
	((uint32_t*)child_stack)[0] = (uint32_t) argblock;
	child_stack -= sizeof(uint32_t);
	(zdt->zoog_thread_create)(
		(zoog_thread_start_f*) cheesy_thread_start, child_stack);
}
