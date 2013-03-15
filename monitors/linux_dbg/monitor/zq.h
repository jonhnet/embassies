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

#include <stdint.h>
#include <pthread.h>

#include "hash_table.h"
#include "linked_list.h"

#if DEBUG_ZUTEX_OWNER
typedef struct {
	pid_t monitor_tid;
	pid_t xax_tid;
	int xax_tid_generation;
} ZQLockOwnerInfo;
#endif // DEBUG_ZUTEX_OWNER

class MThread;
class ZQ {
private:
	pthread_mutex_t mutex;
	LinkedList list;
	uint32_t dbg_zutex_handle;
	uint32_t dbg_last_waker_thread_id;
	uint32_t dbg_last_waker_match_val;	// which is basically dbg anyway
	uint32_t dbg_last_wakee_count;
	MThread *dbg_last_wakee_first_mthread;
#if DEBUG_ZUTEX_OWNER
	ZQLockOwnerInfo zlo;
#endif // DEBUG_ZUTEX_OWNER

public:

	ZQ(MallocFactory *mf, uint32_t dbg_zutex_handle);
	~ZQ();
	#if DEBUG_ZUTEX_OWNER
	void lock(ZQLockOwnerInfo *zlo);
	#else
	void lock();
	#endif
	void unlock();
	void append(MThread *thr);
	void remove(MThread *thr);
	MThread *pop();
	void dbg_note_wake(uint32_t thread_id, uint32_t match_val, ZQ *pwq);
	// uint32_t len();
	// MThread *peek(uint32_t i);
};

class ZQT_Entry {
public:
	uint32_t handle;
	ZQ *zq;

	ZQT_Entry(uint32_t handle, MallocFactory *mf);
		// null mf => key ctor
	~ZQT_Entry();

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);
};

class ZQTable {
private:
	MallocFactory *mf;
	HashTable ht;
	pthread_mutex_t tablelock;

	static void _zqt_free_one(void *v_zqt, void *v_zqte);

public:
	ZQTable(MallocFactory *mf);
	~ZQTable();
	ZQ *lookup(uint32_t *handle);
};
