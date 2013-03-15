#include <stdlib.h>
#include <string.h>
#include "memtracker.h"
#include "bogus_ebp_stack_sentinel.h"
#include "zoog_qsort.h"
#include "cheesy_snprintf.h"
#include "profiler_ifc.h"
#include "gsfree_lib.h"

MTAllocation *mta_copy(MallocFactory *mf, MTAllocation *src)
{
	int size = sizeof(MTAllocation)+sizeof(uint32_t)*src->depth;
	MTAllocation *copy = (MTAllocation *)mf_malloc(mf, size);
	memcpy(copy, src, size);
	return copy;
}

uint32_t addr_hashfn(const void *obj)
{
	MTAllocation *mta = (MTAllocation *) obj;
	return (mta->addr) ^ (mta->addr*131) ^ (mta->addr>>16);
}

int addr_comparefn(const void *o0, const void *o1)
{
	MTAllocation *mta0 = (MTAllocation *) o0;
	MTAllocation *mta1 = (MTAllocation *) o1;
	return mta0->addr - mta1->addr;
}

MemTracker *mt_init(MallocFactory *mf)
{
	MemTracker *mt = (MemTracker *)mf_malloc(mf, sizeof(MemTracker));
	mt->mf = mf;
	mt->depth = 12;
	hash_table_init(&mt->ht, mf, addr_hashfn, addr_comparefn);
	hash_table_init(&mt->site_table, mf, site_hashfn, site_comparefn);
	mt->unmatched_allocs = 0;
	mt->unmatched_frees = 0;
	cheesy_lock_init(&mt->lock);
	return mt;
}

void addr_freefn(void *obj, void *v_mt)
{
	MemTracker *mt = (MemTracker *) v_mt;
	MTAllocation *mta = (MTAllocation *) obj;
	mf_free(mt->mf, mta);
}

void mt_free(MemTracker *mt)
{
	hash_table_remove_every(&mt->ht, addr_freefn, mt);
	mf_free(mt->mf, mt);
}

void mt_trace_alloc(MemTracker *mt, uint32_t addr, uint32_t size)
{
	cheesy_lock_acquire(&mt->lock);
	MTAllocation *mta = collect_fingerprint(mt->mf, mt->depth, 1, NULL);
	mta->addr = addr;
	mta->size = size;

	MTAllocation *dupl = hash_table_lookup(&mt->ht, mta);
	if (dupl!=NULL)
	{
		hash_table_remove(&mt->ht, dupl);
		mf_free(mt->mf, dupl);
		mt->unmatched_allocs += 1;
	}

	hash_table_insert(&mt->ht, mta);

	SiteRecord site_key;
	site_key.mta = mta;
	SiteRecord *site = hash_table_lookup(&mt->site_table, &site_key);
	if (site==NULL)
	{
		site = site_init(mt->mf, mta);
		hash_table_insert(&mt->site_table, site);
	}
	site->calls.alloced += 1;
	site->bytes.alloced += size;
	/*
	safewrite_str("A ");
	mta_print(mta);
	safewrite_str("\n");
	*/

	cheesy_lock_release(&mt->lock);
}

void mt_trace_free(MemTracker *mt, uint32_t addr)
{
	cheesy_lock_acquire(&mt->lock);
	MTAllocation mtakey;
	mtakey.addr = addr;
	MTAllocation *mta = hash_table_lookup(&mt->ht, &mtakey);

	if (mta!=NULL)
	{
		SiteRecord site_key;
		site_key.mta = mta;
		SiteRecord *site = hash_table_lookup(&mt->site_table, &site_key);
		_profiler_assert(site!=NULL);
		site->calls.freed += 1;
		site->bytes.freed += mta->size;

		hash_table_remove(&mt->ht, mta);
		mf_free(mt->mf, mta);
	}
	else
	{
		mt->unmatched_frees += 1;
	}
	cheesy_lock_release(&mt->lock);
}

SiteRecord *site_init(MallocFactory *mf, MTAllocation *mta)
{
	SiteRecord *sr = mf_malloc(mf, sizeof(SiteRecord));
	sr->mf = mf;
	sr->mta = mta_copy(mf, mta);
	sr->calls.alloced = 0;
	sr->calls.freed = 0;
	sr->bytes.alloced = 0;
	sr->bytes.freed = 0;
	return sr;
}

void site_free(SiteRecord *site)
{
	mf_free(site->mf, site->mta);
	mf_free(site->mf, site);
}

uint32_t site_hashfn(const void *obj)
{
	SiteRecord *site = (SiteRecord *) obj;
	return fp_hashfn(site->mta);
}

int site_comparefn(const void *o0, const void *o1)
{
	SiteRecord *site0 = (SiteRecord *) o0;
	SiteRecord *site1 = (SiteRecord *) o1;

	return fp_comparefn(site0->mta, site1->mta);
#if 0
	MTAllocation *mta0 = site0->mta;
	MTAllocation *mta1 = site1->mta;

	// compare to shallowest depth available, and declare
	// a match if they agree. This lets a shallow mta find
	// deep sites in the table.
	int depth;
	if (mta0->depth < mta1->depth)
	{
		depth = mta0->depth;
	}
	else
	{
		depth = mta1->depth;
	}
	int i;
	for (i=0; i<depth; i++)
	{
		int rc = mta0->pc[i] - mta1->pc[i];
		if (rc!=0)
		{
			return rc;
		}
	}
	return 0;
#endif
}

// coalesce every allocation by (abbreviated) stack trace.
void addr_loadtraces(void *obj, void *arg)
{
	LoadTraces *lt = (LoadTraces *) obj;
	MTAllocation *mta = (MTAllocation *) arg;

	_profiler_assert(lt->level <= mta->depth);

	MTAllocation *mtakey = mta_copy(lt->mf, mta);
	mtakey->depth = lt->level;
	MTAllocation *accum = (MTAllocation *) hash_table_lookup(&lt->ht, mtakey);
	if (accum==NULL)
	{
		hash_table_insert(&lt->ht, mtakey);
		accum = mtakey;
		accum->accum_size = 0;
	}
	else
	{
		mf_free(lt->mf, mtakey);
	}
	accum->accum_size += mta->size;
}

// coalesce every site by (abbreviated) stack trace
void site_loadtraces(void *obj, void *arg)
{
	LoadTraces *lt = (LoadTraces *) obj;
	SiteRecord *site = (SiteRecord *) arg;

	_profiler_assert(lt->level <= site->mta->depth);

	MTAllocation *mtakey = mta_copy(lt->mf, site->mta);
	mtakey->depth = lt->level;
	SiteRecord *sitekey = site_init(lt->mf, mtakey);

	SiteRecord *accum_site = (SiteRecord *) hash_table_lookup(&lt->site_table, sitekey);
	if (accum_site==NULL)
	{
		hash_table_insert(&lt->site_table, sitekey);
		accum_site = sitekey;
	}
	else
	{
		mf_free(lt->mf, sitekey);
	}
	accum_site->calls.alloced += site->calls.alloced;
	accum_site->calls.freed += site->calls.freed;
	accum_site->bytes.alloced += site->bytes.alloced;
	accum_site->bytes.freed += site->bytes.freed;
}

void fp_gathertraces(void *obj, void *arg)
{
	LoadTraces *lt = (LoadTraces *) obj;
	MTAllocation *accum = (MTAllocation *) arg;
	
	_profiler_assert(accum->depth == lt->level);
	if (lt->tracespace != NULL)
	{
		lt->tracespace[lt->traceindex] = accum;
	}
	lt->traceindex += 1;
}

int accum_size_comparefun(const void *o0, const void *o1)
{
	MTAllocation **mta0 = (MTAllocation **) o0;
	MTAllocation **mta1 = (MTAllocation **) o1;
	return mta0[0]->accum_size - mta1[0]->accum_size;
}

void mta_print(MTAllocation *mta)
{
	int di;
	for (di=0; di<mta->depth; di++)
	{
		safewrite_hex(mta->pc[di]);
		safewrite_str(" ");
	}
}

void lt_printtraces(LoadTraces *lt)
{
	lt->calls.alloced = 0;
	lt->calls.freed = 0;
	lt->bytes.alloced = 0;
	lt->bytes.freed = 0;

	int ti;
	for (ti=0; ti<lt->tracecount; ti++)
	{
		MTAllocation *mta = lt->tracespace[ti];
		mta_print(mta);
		safewrite_str(": ");
		safewrite_hex(mta->accum_size);
		safewrite_str(" (");
		safewrite_dec(mta->accum_size>>20);
		safewrite_str(" MB) ");

		SiteRecord site_key;
		site_key.mta = mta;
		SiteRecord *site = hash_table_lookup(&lt->site_table, &site_key);
		safewrite_str("live ");
		safewrite_dec(site->calls.alloced - site->calls.freed);
		safewrite_str(" alloced ");
		if (site!=NULL)
		{
			// this one is probably failing because we neutered the mta.
			safewrite_dec(site->calls.alloced);
			safewrite_str(" (");
			safewrite_dec(site->bytes.alloced>>20);
			safewrite_str("MB) freed ");
			safewrite_dec(site->calls.freed);
			safewrite_str(" (");
			safewrite_dec(site->bytes.freed>>20);
		}
		safewrite_str("MB)\n");

		lt->calls.alloced += site->calls.alloced;
		lt->calls.freed += site->calls.freed;
		lt->bytes.alloced += site->bytes.alloced;
		lt->bytes.freed += site->bytes.freed;
	}
}

void mt_dump_traces(MemTracker *mt, int level)
{
	LoadTraces loadtraces, *lt=&loadtraces;
	lt->mf = mt->mf;
	lt->level = level;
	hash_table_init(&lt->ht, mt->mf, fp_hashfn, fp_comparefn);
	hash_table_visit_every(&mt->ht, addr_loadtraces, lt);
	hash_table_init(&lt->site_table, mt->mf, site_hashfn, site_comparefn);
	hash_table_visit_every(&mt->site_table, site_loadtraces, lt);

	// Count.
	lt->tracespace = NULL;
	lt->traceindex = 0;
	hash_table_visit_every(&lt->ht, fp_gathertraces, lt);
	lt->tracecount = lt->traceindex;

	// Fill.
	lt->tracespace = (MTAllocation**)mf_malloc(mt->mf, sizeof(MTAllocation*)*lt->tracecount);
	lt->traceindex = 0;
	hash_table_visit_every(&lt->ht, fp_gathertraces, lt);

	// sort & print
	zoog_qsort(lt->tracespace, lt->tracecount, sizeof(MTAllocation*), accum_size_comparefun);
	lt_printtraces(lt);

	safewrite_str("unmatched_allocs: ");
	safewrite_dec(mt->unmatched_allocs);
	safewrite_str("\n");
	safewrite_str("unmatched_frees: ");
	safewrite_dec(mt->unmatched_frees);
	safewrite_str("\n");

	char buf[100];
	cheesy_snprintf(buf, sizeof(buf), "Total bytes allocated: 0x%08x (%dMB)\n",
		lt->bytes.alloced - lt->bytes.freed,
		(lt->bytes.alloced - lt->bytes.freed)>>20);
	safewrite_str(buf);
}
