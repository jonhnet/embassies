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

#include "linked_list.h"
#include "ZutexWaiter.h"

class ZutexWaiter;

class ZutexQueue
{
public:
	ZutexQueue(MallocFactory *mf, uint32_t guest_hdl);

	ZutexQueue(uint32_t guest_hdl);
		// key constructor

	~ZutexQueue();

	void push(ZutexWaiter *zw);
	ZutexWaiter *pop();
	void remove(ZutexWaiter *zw);
	bool is_empty();

	static uint32_t hash(const void *datum);
	static int cmp(const void *a, const void *b);

	uint32_t dbg_get_guest_hdl() { return guest_hdl; }
	LinkedList *dbg_get_waiters() { return &queue; }

private:
	// Hmm. If these ever got long, I'd say do an AVL tree (now that we
	// have them!). But if they're typically short, a linked list is fine.
	uint32_t guest_hdl;

	bool is_key;
	LinkedList queue;
};
