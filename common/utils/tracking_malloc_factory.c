#if ZOOG_NO_STANDARD_INCLUDES
void *malloc(unsigned size);
void free(void *ptr);
#else
#include <malloc.h>
#endif

#include <stdio.h>
#include <pthread.h>
#include <execinfo.h>
#include <stdlib.h>
#include "malloc_factory.h"
#include "standard_malloc_factory.h"
#include "hash_table.h"
#include "LiteLib.h"

enum { MAX_TRACE = 25 };
typedef struct s_Allocation {
	void* addr;
	uint32_t size;
	void *stack[MAX_TRACE];
	int stack_depth;
} Allocation;

static uint32_t a_hash(const void *datum)
{
	return (uint32_t) ((Allocation*) datum)->addr;
}

static int a_cmp(const void *a, const void *b)
{
	return ((Allocation*) a)->addr - ((Allocation*) b)->addr;
}

static uint32_t a_stack_hash(Allocation *a)
{
	int h = 0;
	int i;
	for (i=0; i<a->stack_depth; i++)
	{
		h = h*131 + (uint32_t) a->stack[i];
	}
	return h;
}

typedef struct s_Tracker {
	pthread_mutex_t mutex;
	HashTable h;
} Tracker;

static Tracker* get_tracker()
{
	static Tracker* g_tracker = NULL;
	if (g_tracker==NULL)
	{
		g_tracker = (Tracker*) malloc(sizeof(Tracker));
		hash_table_init(&g_tracker->h, standard_malloc_factory_init(), a_hash, a_cmp);
		pthread_mutex_init(&g_tracker->mutex, NULL);
	}
	return g_tracker;
}

static void tracker_alloc(void* addr, uint32_t size)
{
	Tracker* tracker = get_tracker();
	pthread_mutex_lock(&tracker->mutex);
	Allocation* a = (Allocation*) malloc(sizeof(Allocation));
	a->addr = addr;
	a->size = size;
	a->stack_depth = backtrace(a->stack, MAX_TRACE);
	hash_table_insert(&tracker->h, a);
	pthread_mutex_unlock(&tracker->mutex);
}

static void tracker_free(void* addr)
{
	Tracker* tracker = get_tracker();
	pthread_mutex_lock(&tracker->mutex);
	Allocation key;
	key.addr = addr;
	Allocation* a = (Allocation*) hash_table_lookup(&tracker->h, &key);
	if (a!=NULL)
	{ 
		hash_table_remove(&tracker->h, a);
		free(a);
	} // else a was already cleared by an earlier reset.
	pthread_mutex_unlock(&tracker->mutex);
}

typedef struct s_Report {
	int count;
	Allocation** ary;
	FILE* fp;
	int idx;
} Report;

static void tracker_report_one(void* user_obj, void* v_a)
{
	Report* r = (Report*) user_obj;
	Allocation* a = (Allocation*) v_a;
	lite_assert(r->idx < r->count);
	r->ary[r->idx++] = a;
}

int report_compar(const void* v_a, const void* v_b)
{
	Allocation** a = (Allocation**) v_a;
	Allocation** b = (Allocation**) v_b;
	return - ((*a)->size - (*b)->size);
}

void tracker_report()
{
	Tracker* tracker = get_tracker();
	pthread_mutex_lock(&tracker->mutex);

	Report report;
	report.count = tracker->h.count;
	report.ary = (Allocation**) malloc(report.count*sizeof(Allocation*));
	report.idx = 0;
	hash_table_visit_every(&tracker->h, tracker_report_one, &report);
	lite_assert(report.idx == report.count);

	qsort(report.ary, report.count, sizeof(report.ary[0]), report_compar);

	report.fp = fopen("/tmp/tracker.output", "w");
	int i;
	for (i=0; i<report.count; i++)
	{
		Allocation* a = report.ary[i];
		fprintf(report.fp, "addr %08x size %08x stack_hash %08x\n", (uint32_t) a->addr, a->size, a_stack_hash(a));
		if (i<10) {
			char** syms = backtrace_symbols(a->stack, a->stack_depth);
			int d;
			for (d=0; d<a->stack_depth; d++)
			{
				fprintf(report.fp, "    %s\n", syms[d]);
			}
			free(syms);
		}
	}
	fclose(report.fp);

	pthread_mutex_unlock(&tracker->mutex);
}

static void tracker_free_one(void* user_obj, void* v_a)
{
	Allocation* a = (Allocation*) v_a;
	free(a);
}

void tracker_reset()
{
	Tracker* tracker = get_tracker();
	pthread_mutex_lock(&tracker->mutex);
	hash_table_remove_every(&tracker->h, tracker_free_one, NULL);
	pthread_mutex_unlock(&tracker->mutex);
}

static void *tmf_malloc(MallocFactory *mf, size_t size)
{
	void* addr = malloc(size);
	tracker_alloc(addr, size);
	return addr;
}

static void tmf_free(MallocFactory *mf, void *ptr)
{
	tracker_free(ptr);
	free(ptr);
}

static MallocFactory _tracking_malloc_factory = { tmf_malloc, tmf_free };

#ifdef __cplusplus
extern "C" {
#endif
MallocFactory *tracking_malloc_factory_init()
{
	return &_tracking_malloc_factory;
}
#ifdef __cplusplus
}
#endif
