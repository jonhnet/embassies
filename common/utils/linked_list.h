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

#include "pal_abi/pal_basic_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "malloc_factory.h"

typedef struct s_linked_list_link {
	void *datum;
	struct s_linked_list_link *next;
	struct s_linked_list_link *prev;
} LinkedListLink;

typedef struct {
	MallocFactory *mf;
	LinkedListLink *head;
	LinkedListLink *tail;
	int count;
} LinkedList;

void linked_list_init(LinkedList *ll, MallocFactory *mf);

void linked_list_insert_head(LinkedList *ll, void *datum);

void linked_list_insert_tail(LinkedList *ll, void *datum);

void *linked_list_remove_head(LinkedList *ll);
	// returns NULL if list is empty.

void *linked_list_remove_tail(LinkedList *ll);

void linked_list_remove(LinkedList *ll, void *datum);
	// Just exactly as O(n) as you imagine.

void linked_list_remove_middle(LinkedList *ll, LinkedListLink *link);
	// semi-disgusting interface assumes caller can peek into our structure
	// this frees link, but caller keeps link->datum
LinkedListLink *linked_list_find(LinkedList *ll, void *datum);
	// eases the pain of the prior call, a little.

typedef bool (visit_f)(const void *datum, void *user_data);
	// return 'false' to interrupt scanning
	// TODO reverse order of arguments, so that an object's 'this' appears
	// first. (Fixing this would make this ADT consistent with hash_table.)

void linked_list_visit_all(LinkedList *ll, visit_f *visit, void *user_data);
	// don't mutate list during scan, or you'll confuse me.

typedef LinkedListLink *LinkedListIterator;

static inline void ll_start(LinkedList *ll, LinkedListIterator *li)
	{ *li = ll->head; }
static inline bool ll_has_more(LinkedListIterator *li)
	{ return (*li)!=NULL; }
static inline void ll_advance(LinkedListIterator *li)
	{ *li = (*li)->next; }
static inline void *ll_read(LinkedListIterator *li)
	{ return (*li)->datum; }
static inline void ll_remove(LinkedList *ll, LinkedListIterator *li)
	{
		LinkedListLink *next = (*li)->next;
		linked_list_remove_middle(ll, (*li));
		(*li)=next;
	}

#ifdef __cplusplus
}
#endif
