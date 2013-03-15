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

#include "malloc_factory.h"
#include "pal_abi/pal_types.h"

#ifdef __cplusplus
extern "C" {
#endif


#define HLMAGIC0 0x33445566
#define HLMAGIC1 0x34563456
typedef struct s_hash_link {
	uint32_t magic0;
	struct s_hash_link *next;
	void *datum;
	uint32_t magic1;
} HashLink;

typedef uint32_t (HashFunc)(const void *datum);
typedef int (CmpFunc)(const void *a, const void *b);
	// return 0 for equality
typedef void (ForeachFunc)(void *user_obj, void *a);

typedef struct {
	HashLink *buckets;
	int num_buckets;
	int count;
} Subtable;

typedef struct {
	MallocFactory *mf;
	HashFunc *hash_func;
	CmpFunc *cmp_func;
	Subtable *old_table;
	int next_old_bucket;	// used to evict old table incrementally
	Subtable *new_table;
	int count;
	uint32_t visiting;
} HashTable;

void hash_table_init(HashTable *ht, MallocFactory *mf, HashFunc *hash_func, CmpFunc *cmp_func);
void hash_table_free(HashTable *ht);
	// fails if ht not empty
void hash_table_free_discard(HashTable *ht);
	// discards all stored data
	// (no freeing occurs; caller must have some other deallocation strategy)

void hash_table_insert(HashTable *ht, void *datum);
	// asserts item not present
void hash_table_union(HashTable *ht, void *datum);
	// no-op if item not present
	// (no freeing occurs; caller must have some other deallocation strategy)
void *hash_table_lookup(HashTable *ht, void *key);
void hash_table_remove(HashTable *ht, void *key);
	// asserts key is present.
	// Caller is responsible for freeing the datum's memory,
	// implying caller must have already lookup'd the datum.
void hash_table_visit_every(HashTable *ht, ForeachFunc *ff, void *user_obj);
void hash_table_remove_every(HashTable *ht, ForeachFunc *ff, void *user_obj);

void hash_table_subtract(HashTable *ht0, HashTable *ht1);
	// for each member of ht1 that also appears in ht0, remove ht1 from ht0.
	// (no freeing occurs; caller must have some other deallocation strategy)

uint32_t hash_buf(const void *buf, int len);
uint32_t cmp_bufs(const void *buf0, int len0, const void *buf1, int len1);
	// utility functions for building hash functions.

#ifdef __cplusplus
}
#endif
