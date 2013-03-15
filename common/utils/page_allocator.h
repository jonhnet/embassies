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
#include "cheesylock.h"
#include "abstract_aligned_mmap.h"

#define PA_LG_PAGE_SIZE			(12)
#define PA_PAGE_SIZE			(1<<PA_LG_PAGE_SIZE)
#define PA_LG_CHUNK_SIZE		(23)
#define PA_CHUNK_SIZE			(1<<PA_LG_CHUNK_SIZE)
#define PA_LG_BIN_CAPACITY		(2)

/*
Page allocator strategy:
For allocations < 1 page, we give you 1 page. (sorry, we're not good at those.)
Allocations are chunked together in CHUNK_SIZE, so that we can
round down to find the accounting header. Past CHUNK_SIZE, we never
allocate more than one region per bin (backing allocation).

TODO this implementation never frees anything ever.
TODO the bins are searched linearly, so if they get big, we get needless O(n^2).
*/

struct s_page_allocator_arena;
typedef struct s_page_allocator_arena PageAllocatorArena;

struct s_page_allocator_bin;
typedef struct s_page_allocator_bin PageAllocatorBin;

struct s_page_allocator_bin {
	uint32_t magic;
	PageAllocatorArena *arena;
	uint32_t lg_alloc_size;
	uint32_t capacity;
	uint32_t count;
	PageAllocatorBin *next;
	bool occupied[1<<PA_LG_BIN_CAPACITY];
};

struct s_page_allocator_arena {
	CheesyLock mutex;
	PageAllocatorBin *binHead[33];
	AbstractAlignedMmapIfc *mmapifc;
};

void page_allocator_init_arena(PageAllocatorArena *arena, AbstractAlignedMmapIfc *mmapifc);
void *page_alloc(PageAllocatorArena *arena, uint32_t length);
void page_free(void *ptr);

