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

#include <pthread.h>
#include "zq.h"
#include "hash_table.h"
#include "linked_list.h"

class Tx;

class MThread {
public:
	enum CtorType { ACTUAL, KEY };

private:
	int thread_id;
	CtorType type;
	pthread_mutex_t mutex;
	Tx *tx;	// the transaction that I'm blocked in.

	LinkedList zqs;

public:
	MThread(int thread_id, CtorType type, MallocFactory *mf);
	~MThread();

	void wait_setup();
	void wait_push(ZQ *zq);
	void wait_complete(Tx *tx);

	void wake_setup();
	ZQ *wake_pop_zq();
	Tx *wake_get_tx();
	void wake_complete();

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);
};

class ThreadTable {
private:
	MallocFactory *mf;
	HashTable ht;
	pthread_mutex_t mutex;
	
	static void _free_thread(void *user_obj, void *a);

public:
	ThreadTable(MallocFactory *mf);
	~ThreadTable();
	MThread *lookup(int thread_id);
};
