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

/* How to run the profiler, from gdb:

shell ../scripts/debuggershelper.py
source addsyms
set profiler.dump_flag=1
cont

... reset it with:
set profiler.reset_flag = false;

*/

#define PROFILER_ENABLED 1

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "hash_table.h"
#include "malloc_factory.h"
#include "cheesylock.h"

typedef struct {
	bool sampling_enabled;
	bool dump_flag;
	bool reset_flag;
	bool dump_unlimited;
	char epoch_name[200];
	int handler_called;
} ProfilerConfig;
extern ProfilerConfig profiler;

typedef struct {
	MallocFactory *mf;
	CheesyLock lock;
	HashTable ht;
	int depth;
	pid_t dump_thread_tid;
} Profiler;

void profiler_init(Profiler *profiler, MallocFactory *mf);

#ifdef __cplusplus
}
#endif // __cplusplus
