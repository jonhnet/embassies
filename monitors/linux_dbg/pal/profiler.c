#include <alloca.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <sched.h>	// clone
#include <sys/ucontext.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>	// __NR_gettid
#include <time.h>
#include <fcntl.h>

#include "MTAllocation.h"	// fingerprint collection & hashing funcs
#include "profiler.h"
#include "bogus_ebp_stack_sentinel.h"
#include "cheesy_snprintf.h"
#include "c_qsort.h"
#include "LiteLib.h"
#include "profiler_ifc.h"

/*
A simple sampling profiler.

NB the current implementation is broken in a few ways.

The most important is that it has a segfault-y race. It walks the ebp stack,
which isn't always valid. Two things make ebp become meaningless: the two places
where we go through a linux-syscall calling convention.

The first is gsfree_syscall (a real linux syscall, at the bottom
of this PAL). We "fix" that by putting a marker (BOGUS_EBP_STACK_SENTINEL)
on the stack, so that we can tell when we're in that case, and look
up the correct ebp.

However, there are a couple of instructions during which ebp is broken
but the stack isn't marked correctly. If we happen to snap a sample at
that moment, the sampler will segfault. Wah wah.

The second place is in the transition from static_rewriter's libc through
the syscall interface (but not actually a syscall) into xinterpose().
We avoid this trap by, in the stack-walker, looking for ebp jumps beyond
some limit (10k, presently). It's not bulletproof, but it's a pretty good
bet and hasn't failed yet.

TODO One way to make it more robust (but slower): track (or look up) memory
ranges; verify that the first ebp is in some range, and then never let it
stray out of that range (reasonably assuming the stack lives within one
contiguously-allocated range).

*/

int dump_wait_thread(void *v_prof);

Profiler *g_profiler;
	// no user_data for signal handler :v(

void profile_alarm_handler(int sig, siginfo_t *info, void *v_uc)
{
	if (!profiler.sampling_enabled)
	{
		return;
	}

	profiler.handler_called += 1;

	ucontext_t *uc = (ucontext_t *) v_uc;
	_profiler_assert(sig==SIGVTALRM);

	Profiler *prof = g_profiler;
	
	if (profiler_my_id() == prof->dump_thread_tid)
	{
		// don't profile dump thread, or we'll deadlock on the nonreentrant
		// cheesylock.
		return;
	}

	MTAllocation *mta;

	uint32_t *esp = (uint32_t*) uc[0].uc_mcontext.gregs[7];
	void *start_ebp;
	if (esp[0] == BOGUS_EBP_STACK_SENTINEL)
	{
		// See gsfree_syscall.S for stack arrangement.
		start_ebp = (void*) esp[1];
	}
	else
	{
		start_ebp = (void*) uc[0].uc_mcontext.gregs[6];
	}
	mta = collect_fingerprint(prof->mf, prof->depth, 0, start_ebp);

	mta->accum_size = 1;
		// NB john abashadly recycling MTA object, reusing fields
		// for a different meaning here. Too tired to factor correctly
		// right now with GI Joe Polymorphing Action.
	cheesy_lock_acquire(&prof->lock);
	MTAllocation *existing = hash_table_lookup(&prof->ht, mta);
	if (existing!=NULL)
	{
		existing->accum_size += 1;
		mf_free(prof->mf, mta);
	}
	else
	{
		hash_table_insert(&prof->ht, mta);
	}
	cheesy_lock_release(&prof->lock);
}

ProfilerConfig profiler;

void profiler_init(Profiler *prof, MallocFactory *mf)
{
#if !PROFILER_ENABLED
	lite_memset(prof, 0, sizeof(*prof));
	return;
#endif
	prof->mf = mf;
	cheesy_lock_init(&prof->lock);
	hash_table_init(&prof->ht, mf, fp_hashfn, fp_comparefn);
	prof->depth = 4;

	g_profiler = prof;

	struct sigaction act;
	act.sa_sigaction = profile_alarm_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;	// want ucontext
	sigaction(SIGVTALRM, &act, NULL);

	profiler.sampling_enabled = true;
	profiler.dump_flag = false;
	profiler.reset_flag = false;
	profiler.dump_unlimited = false;
	lite_strcpy(profiler.epoch_name, "default");
	profiler.handler_called = 0;

//	int stack_size = 8<<12;
	int stack_size = 128<<10;
		// when the data set gets big, the qsort gets deep, and we need
		// a big stack.
	prof->dump_thread_tid =
		profiler_thread_create(dump_wait_thread, prof, stack_size);

	struct itimerval itv;
	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 5000;
	itv.it_value = itv.it_interval;
	setitimer(ITIMER_VIRTUAL, &itv, NULL);
}

typedef struct {
	MTAllocation **mta_ary;
	int capacity;
	int next;
	int total_visits;
} ProfilerDumper;

void pd_add_one(void *v_pd, void *v_mta)
{
	MTAllocation *mta = (MTAllocation *) v_mta;
	ProfilerDumper *pd = (ProfilerDumper *) v_pd;
	_profiler_assert(pd->next < pd->capacity);
	pd->mta_ary[pd->next++] = v_mta;
	pd->total_visits += mta->accum_size;
}

int mta_count_neg_cmp(const void *v_a, const void *v_b)
{
	MTAllocation *a = ((MTAllocation **) v_a)[0];
	MTAllocation *b = ((MTAllocation **) v_b)[0];
	return -(a->accum_size - b->accum_size);
}

void profiler_dump(Profiler *prof)
{
	ProfilerDumper pd;

	cheesy_lock_acquire(&prof->lock);
	pd.capacity = prof->ht.count;
	pd.mta_ary = (MTAllocation **)
		mf_malloc(prof->mf, sizeof(MTAllocation *) * pd.capacity);
		// NB can't use alloca here because it changes the frame pointer (!),
		// confusing a debug check in cheesy_lock_*
	pd.next = 0;
	pd.total_visits = 0;
	hash_table_visit_every(&prof->ht, pd_add_one, &pd);
	_profiler_assert(pd.next == pd.capacity);
	c_qsort(pd.mta_ary, pd.capacity, sizeof(MTAllocation*), mta_count_neg_cmp);
	
	char filename[200];
	cheesy_snprintf(filename, sizeof(filename), "profiler.%s.out", profiler.epoch_name);
	int fd = _profiler_open(filename, O_WRONLY|O_CREAT, 0644);

	int i;
	for (i=0; i<pd.capacity && (profiler.dump_unlimited || i<10); i++)
	{
		MTAllocation *mta = pd.mta_ary[i];
		char buf[200];
		cheesy_snprintf(buf, sizeof(buf),
			"%4d (%4.2f%%)\n     EIP(0x%08x)\n     EIP(0x%08x)\n     EIP(0x%08x)\n     EIP(0x%08x)\n",
			mta->accum_size,
			(mta->accum_size*100.0)/pd.total_visits,
			mta->pc[0],
			mta->pc[1],
			mta->pc[2],
			mta->pc[3]
			);
		_profiler_write(fd, buf, lite_strlen(buf));
	}
	_profiler_close(fd);
	mf_free(prof->mf, pd.mta_ary);
	cheesy_lock_release(&prof->lock);
}

void pr_remove_one(void *v_prof, void *v_mta)
{
	Profiler *prof = (Profiler *) v_prof;
	MTAllocation *mta = (MTAllocation *) v_mta;
	mf_free(prof->mf, mta);
}

void profiler_reset(Profiler *prof)
{
	cheesy_lock_acquire(&prof->lock);
	hash_table_remove_every(&prof->ht, pr_remove_one, prof);
	cheesy_lock_release(&prof->lock);
}

int dump_wait_thread(void *v_prof)
{
	Profiler *prof = (Profiler *) v_prof;
	while (1)
	{
		struct timespec req;
		req.tv_sec = 0;
		req.tv_nsec = 100*1000*1000;
		_profiler_nanosleep(&req);

		if (profiler.dump_flag)
		{
			profiler_dump(prof);
			profiler.dump_flag = false;
		}
		if (profiler.reset_flag)
		{
			profiler_reset(prof);
			profiler.reset_flag = false;
		}
	}
}
