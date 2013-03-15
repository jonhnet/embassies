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

#include <stdbool.h>
#include <stdint.h>
#include "pal_abi/pal_abi.h"
#include "abstract_mmap.h"
#include "malloc_trace.h"

// When we load multiple intances of libc in the same process,
// they compete over the brk().

// NB we don't lock this data structure because the whole execution model
// depends on the assumption that the orchestrator sequences the
// arrival of new brk requests.
// (No synchronization is needed for
// interleaving of old brk requests. There's a concurrent access to
// Xbrk.count in an assert, but we assume count is an atomically-accessible
// word.)

typedef struct {
	uint8_t *start;
	uint8_t *current;
	uint8_t *end;
	const char *dbg_desc;
} XbrkRange;

#define NUM_RANGES 8

class Xbrk {
private:
	ZoogDispatchTable_v1 *zdt;
	AbstractMmapIfc *ami;
	MallocTraceFuncs *mtf;
	XbrkRange ranges[NUM_RANGES];
	uint32_t capacity;
	uint32_t count;
	uint32_t next_new_brk;

public:
	void init(ZoogDispatchTable_v1 *zdt, AbstractMmapIfc *ami, MallocTraceFuncs *mtf);
		// not a constructor, because owner isn't yet arranged
		// C++ily.
	void allow_new_brk(uint32_t size, const char *dbg_desc);
	int wait_until_all_brks_claimed();
	void *new_brk();
	void *more_brk(void *new_location);
	void debug_dump_allocations();
};


