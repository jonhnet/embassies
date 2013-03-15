#include "paltest.h"

#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"
#include "cheesy_thread.h"
#include "concrete_mmap_xdt.h"
#include "cheesy_malloc_factory.h"
#include "xax_util.h"
#include "LiteLib.h"

typedef struct {
	ZoogDispatchTable_v1 *zdt;
	debug_logfile_append_f *s_debuglog;
	bool done;
} ThreadTest;

#define NUM_THREADS	10

//static debug_logfile_append_f *s_debuglog = 0;

void thread_test_thread(void *v_arg)
{
	ThreadTest *tt = (ThreadTest*) v_arg;
	int i;
	for (i=0; i<50000; i++)
	{
	}
	(tt->s_debuglog)("stderr", "a thread finishes.\n");
	tt->done = true;
	(tt->zdt->zoog_thread_exit)();
}

void threadtest(ZoogDispatchTable_v1 *zdt)
{
	ThreadTest tt;
	tt.zdt = zdt;
	tt.s_debuglog = (zdt->zoog_lookup_extension)("debug_logfile_append");

	int i;
	for (i=0; i<40; i++)
	{
		tt.done = false;
		// using cheesy_thread_create leaks thread stacks. Meh.
		cheesy_thread_create(zdt, thread_test_thread, &tt, 4096);
		while (!tt.done)
			{ }
	}
	(tt.s_debuglog)("stderr", "threadtest done.\n");
}
