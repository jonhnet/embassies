#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "CallCounts.h"

static bool dump_flag = false;
static void dump_call_counts_s(int sig)
{
	dump_flag = true;
}

int CallCounts::dump_wait_thread(void *v_cc)
{
	CallCounts* cc = (CallCounts*) v_cc;
	while (1)
	{
		if (dump_flag)
		{
			cc->dump();
			dump_flag = false;
		}
		sleep(1);
	}
	return 0;
}

CallCounts::CallCounts()
{
	memset(&call_count, 0, sizeof(call_count));

#if USE_PTHREADS
	pthread_create(&dump_thread, NULL, ((void*)(void*))dump_wait_thread, this);
#endif // USE_PTHREADS

	signal(SIGHUP, dump_call_counts_s);
}

void CallCounts::tally(uint32_t idx)
{
	assert(idx < MAX_INDEX);
	call_count[idx]++;
}

void CallCounts::dump()
{
	FILE *fp = fopen("call_counts", "w");
	for (uint32_t i=0; i<MAX_INDEX; i++)
	{
		fprintf(fp, "call %d %d\n", i, call_count[i]);
	}
	fclose(fp);
	fprintf(stderr, "Dumped to call_counts\n");
}
