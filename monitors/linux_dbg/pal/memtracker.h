/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#pragma once

/*
 How to use memory profiler (memtracker):

Enable appropriate -D flag in common/utils/malloc_trace.h:

#define MALLOC_TRACE_MALLOC 1
	I don't think this is being used.

#define MALLOC_TRACE_MMAP 1
	Traces mmap/munmap calls across the XPE layer

#define MALLOC_TRACE_CHEESY 1
	Traces cheesymallocs (made by things in the Lite context)

and in monitors/linux_dbg/pal/xaxInvokeLinux.c:
#define XIL_TRACE_ALLOC 1
	Traces all pal_abi allocate_memory() calls, which should account
	for everything except startup pal image and any XNBs mapped in.

break break_here_for_memtracker
cont
p dump_traces(tl_{malloc,mmap,cheesy,xil}, 8)


 */

#include <stdint.h>
#include "hash_table.h"
#include "malloc_factory.h"
#include "cheesylock.h"
#include "MTAllocation.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void mta_print(MTAllocation *mta);

// Track the allocation history of one call site, through the
// whole application's run. These are never freed, but the number
// of instances is bound by the number of unique allocation stack
// traces.
typedef struct {
	uint32_t alloced;
	uint32_t freed;
} History;

typedef struct {
	MallocFactory *mf;
	MTAllocation *mta;
	History calls;
	History bytes;
} SiteRecord;

SiteRecord *site_init(MallocFactory *mf, MTAllocation *mta);
void site_free(SiteRecord *site);
uint32_t site_hashfn(const void *obj);
int site_comparefn(const void *o0, const void *o1);


typedef struct {
	MallocFactory *mf;
	int depth;
	HashTable ht;	// maps MTAs
	HashTable site_table;	// maps SiteRecords
	int unmatched_allocs;
	int unmatched_frees;
	CheesyLock lock;
} MemTracker;

MemTracker *mt_init(MallocFactory *mf);
void mt_free(MemTracker *mt);
void mt_trace_alloc(MemTracker *mt, uint32_t addr, uint32_t size);
void mt_trace_free(MemTracker *mt, uint32_t addr);

typedef struct
{
	MallocFactory *mf;
	HashTable ht;			// allocs coalesced to reduced level
	HashTable site_table;	// sited coalasced to reduced level
	int level;
	MTAllocation **tracespace;
	int traceindex;
	int tracecount;

	History bytes;
	History calls;
} LoadTraces;

void mt_dump_traces(MemTracker *mt, int level);

#ifdef __cplusplus
}
#endif // __cplusplus
