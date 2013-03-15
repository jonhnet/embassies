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
/*
 * \brief  trampoline from Genode::Thread into a zoog app's thread
 * \author Jon Howell
 * \date   2011-12-27
 */

#pragma once

#include <base/thread.h>
#include "ZutexWaiter.h"
#include "core_regs.h"

class ThreadTable;

class ZoogThread : public Genode::Thread<128>
{
private:
	void *eip;
	void *esp;
	ThreadTable *thread_table;
	ZutexWaiter _waiter;

	Core_x86_registers core_regs;
	static uint32_t core_thread_id_allocator;
	uint32_t _core_thread_id;

public:
	ZutexWaiter *waiter() { return &_waiter; }

	ZoogThread(ThreadTable *thread_table, SyncFactory *sf);
		// main thread constructor
	ZoogThread(void *eip, void *esp, ThreadTable *thread_table, SyncFactory *sf);
		// new thread constructor
	void entry();

	void set_core_state();
	void clear_core_state();
	bool core_state_valid();
	void fudge_core_state(uint32_t eip, uint32_t esp);

	Core_x86_registers *get_core_regs();
	uint32_t core_thread_id();
};

