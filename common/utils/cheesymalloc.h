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

#include "cheesy_malloc_config.h"

#ifndef DEBUG_MALLOC_EMPTY 
#error Please #define DEBUG_MALLOC_EMPTY to 0 or 1
#error to say whether you'd like to burn time 0xfefefefe'ing memory on
#error alloc and free.
#error example: #define DEBUG_MALLOC_EMPTY 1
#endif

#ifndef CM_NUM_BINS
#error Please #define CM_NUM_BINS to define the largest allocation size
#error (in bits) you want handled buddy-style; larger allocations go
#error straight to underlying allocator.
#error example: #define CM_NUM_BINS (12)
#endif

// We have a cheesymalloc to deal with allocations from our non-p-threads;
// see refactoring-notes.txt, /cheesymalloc [2010.08.26].

#include "cheesylock.h"
#include "abstract_mmap.h"

struct s_cheesy_malloc_arena;

typedef struct s_cheesy_malloc_block {
	uint32_t guard0;
	uint32_t length;	// length including this header
	uint32_t is_free;
	uint32_t acct_user_len;	// length caller thinks he has (used for accounting only)
	struct s_cheesy_malloc_arena *arena;
	struct s_cheesy_malloc_block *next;
	uint32_t guard1;
} CheesyMallocBlock;

// TODO can tighten this block quite a bit.
// is_free, guard0, guard1, guard2
// are only ever used for asserts; can be #if'd out.
// acct_user_len is only used for accounting, can also be #if'd out.
// next can be stored inside the user area (if we enfore a minimum
// allocation size in cm_log2).
// That leaves only length & arena for necessary overhead.

typedef struct s_cheesy_malloc_tail {
	uint32_t guard2;
} CheesyMallocTail;

typedef struct {
	uint32_t bytes_mmapped;
		// bytes_mmapped should equal sum of those below.

	// every mmap'd byte is either in a block or in mmap_fragmentation
	uint32_t bytes_mmap_fragmented;
		// bytes in mmapped regions not assigned to blocks
	
	// every block is either on the free list (all of its bytes),
	// or in an allocated block.
	uint32_t bytes_in_free_blocks;
		// bytes in blocks on the free list
		// bytes in a block in excess of client's request

	// every byte in an allocated block is billed either to the block header,
	// the actual user bytes_allocated, or block_fragmented
	uint32_t bytes_header;
		// bytes in CheesyMallocBlock and CheesyMallocTails of
		// space owned by clients.
	uint32_t bytes_block_fragmented;
		// bytes in the usable space of the block that the client
		// doesn't need.
	uint32_t bytes_allocated;
		// bytes in blocks owned by client; should agree with
		// tl_cheesy accounting.
} CMAccounting;

typedef struct s_cheesy_malloc_arena {
	CheesyLock mutex;
	CheesyMallocBlock binHead[CM_NUM_BINS];
	CMAccounting accounting;
	AbstractMmapIfc *mmapifc;
} CheesyMallocArena;

void cheesy_malloc_init_arena(CheesyMallocArena *arena, AbstractMmapIfc *mmapifc);
void *cheesy_malloc(CheesyMallocArena *arena, uint32_t length);
void cheesy_free(void *ptr);

#ifdef __cplusplus
}
#endif

