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

#include "hash_table.h"	// CmpFunc
#include "linked_list.h"

typedef void (lru_free_f)(void *obj);

typedef struct {
	LinkedList ll;
	int max_size;
	lru_free_f *lru_free;
	CmpFunc *cmp_func;
} LRUCache;

void lrucache_init(LRUCache *lru, MallocFactory *mf, int max_size, lru_free_f *lru_free, CmpFunc cmp_func);
void *lrucache_find(LRUCache *lru, void *key);
void lrucache_insert(LRUCache *lru, void *item);
void lrucache_free(LRUCache *lru);
