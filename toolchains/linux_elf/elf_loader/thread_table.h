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

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdbool.h>

#include "malloc_factory.h"
#include "pal_abi/pal_abi.h"
#include "zutex_sync.h"

struct s_thread_start_block;
typedef struct s_thread_start_block ThreadStartBlock;

typedef struct {
	pid_t tid;
	ThreadStartBlock *tsb;
} ThreadTableEntry;


// Lousiest hash table you've ever seen.

typedef struct {
	MallocFactory *mf;
	ThreadTableEntry *entries;
	int capacity;
	int pseudo_tid;

	ZoogDispatchTable_v1 *zdt;
	ZMutex table_mutex;
} ThreadTable;

void thread_table_init(ThreadTable *tt, MallocFactory *mf, ZoogDispatchTable_v1 *zdt);
void thread_table_insert(ThreadTable *tt, pid_t tid, ThreadStartBlock *tsb);
bool thread_table_remove(ThreadTable *tt, pid_t tid, ThreadStartBlock **out_tsb);
int thread_table_allocate_pseudotid(ThreadTable *tt);

#ifdef __cplusplus
}
#endif
