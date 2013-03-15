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

#include "hash_table.h"	// defn CmpFunc
#include "cheesylock.h"

// A good interface to implement with, say, a balanced tree
// (eg. red-black or avr) or skip list. Not that that's what I'm doing.

typedef struct {
	MallocFactory *mf;
	CmpFunc *cmp_func;

	void **data;
	int count;
	int capacity;
	CheesyLock mutex;
} OrderedCollection;

void ordered_collection_init(OrderedCollection *oc, MallocFactory *mf, CmpFunc *cmp_func);
void ordered_collection_insert(OrderedCollection *oc, void *datum);
	// asserts no equal key already present
void *ordered_collection_lookup_ge(OrderedCollection *oc, void *key);
	// returns a datum that compares equal-to-or-greater-than key
void *ordered_collection_lookup_le(OrderedCollection *oc, void *key);
	// returns a datum that compares equal-to-or-less-than key
void *ordered_collection_lookup_eq(OrderedCollection *oc, void *key);
	// returns a datum that compares equal-to key
void ordered_collection_remove(OrderedCollection *oc, void *key);
	// removes a datum compares equal-to key.
	// Caller is responsible for freeing the datum's memory,
	// implying caller must have already lookup'd the datum.

