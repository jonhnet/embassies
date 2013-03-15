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

#include "hash_table.h"
#include "ZutexQueue.h"
#include "SyncFactory.h"
#include "pal_abi/pal_zutex.h"

class ZutexTable
{
public:
	ZutexTable(MallocFactory *mf, SyncFactory *sf);


	void wait(ZutexWaitSpec *specs, uint32_t count, ZutexWaiter *waiter);

	uint32_t wake(uint32_t guest_zutex, Zutex_count n_wake);

private:
	MallocFactory *mf;
	HashTable table;
	SyncFactoryMutex *mutex;

	void _lock();
	void _unlock();
	static void _free_zq(void *v_this, void *v_zq);
	ZutexQueue *_get_queue(uint32_t guest_hdl);
		// returns null if no hdl exists
	ZutexQueue *_get_or_create_queue(uint32_t guest_hdl);
	static void _enumerate(void *v_ll, void *v_zq);
#if 0
	void dbg_dump_waiter(ZutexWaiter *zw);
	void dbg_sanity(const char *desc);
#endif
};

