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

	CheesyLock lock;
	int running_threads;
} SegTest;

typedef struct {
	int id;
	int loc[4];
	SegTest *segtest;
} SegTestThreadState;

#define NUM_THREADS	10

//static debug_logfile_append_f *s_debuglog = 0;

void seg_test_thread(void *v_arg)
{
	SegTestThreadState *state = (SegTestThreadState *) v_arg;

	char msg[200];
	lite_strcpy(msg, "seg_test_thread _\n");
	msg[16] = 'A'+state->id;
	(state->segtest->s_debuglog)("stderr", msg);

	int j;
	for (j=0; j<4; j++)
	{
		(state->segtest->zdt->zoog_x86_set_segments)(0, (uint32_t) (&state->loc[j]));
		int value = 0;
		int count;
		for (count=0; count < (1<<24); count++)
		{
			value+=state->id;
			state->loc[j] = value;
			state->loc[(j+1)%4] = 6;
			int view0 = state->loc[j];
			int view1 = get_gs_base();
			if (view0!=view1)
			{
				test_fail();
			}
		}

		lite_strcpy(msg, "thread _ round  _\n");
		msg[7] = '0'+j;
		msg[16] = 'A'+state->id;
		(state->segtest->s_debuglog)("stderr", msg);
	}

	cheesy_lock_acquire(&state->segtest->lock);
	state->segtest->running_threads -= 1;
	cheesy_lock_release(&state->segtest->lock);

	(state->segtest->zdt->zoog_thread_exit)();
}

void segtest(ZoogDispatchTable_v1 *zdt)
{
	SegTest segtest;
	segtest.zdt = zdt;
	segtest.s_debuglog = (zdt->zoog_lookup_extension)("debug_logfile_append");
	cheesy_lock_init(&segtest.lock);

	// Big ugly boilerplate to get from zdt to a MallocFactory.
	ConcreteMmapXdt cmx;
	concrete_mmap_xdt_init(&cmx, zdt);
	CheesyMallocArena ca;
	cheesy_malloc_init_arena(&ca, &cmx.ami);
	CheesyMallocFactory cmf;
	cmf_init(&cmf, &ca);

	segtest.running_threads = NUM_THREADS;
	SegTestThreadState states[NUM_THREADS];
	int thread_idx;
	for (thread_idx=0; thread_idx<NUM_THREADS; thread_idx++)
	{
		states[thread_idx].id = thread_idx;
		int j;
		for (j=0; j<4; j++)
		{
			states[thread_idx].loc[j] = 0;
		}
		states[thread_idx].segtest = &segtest;
		cheesy_thread_create(zdt, seg_test_thread, &states[thread_idx], 8192);
	}

	bool done = false;
	int last_running_threads = -1;
	while (!done)
	{
		cheesy_lock_acquire(&segtest.lock);
		int running_threads = segtest.running_threads;
		cheesy_lock_release(&segtest.lock);
		done = (running_threads == 0);
		if (running_threads != last_running_threads)
		{
			char msg[80];
			lite_strcpy(msg, "X threads to go\n");
			msg[0] = running_threads+'0';
			(segtest.s_debuglog)("stderr", msg);
			last_running_threads = running_threads;
		}
	}
	(segtest.s_debuglog)("stderr", "segtest done.\n");
}
