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
#include "SyncFactory.h"
#include "VCPUYieldControlIfc.h"

class ZutexQueue;
class ZoogVCPU;

class ZutexWaiter
{
public:
	ZutexWaiter(MallocFactory *mf, SyncFactory *sf, ZoogVCPU *dbg_vcpu);
	void wait(VCPUYieldControlIfc *vyci);
	void wake();
	void reset_event();

	void append_list(ZutexQueue *zq);
	LinkedList *get_zq_list();
	int dbg_get_vcpu_zid();

private:
	SyncFactoryEvent *event;
	LinkedList waiting_on_queues;
	ZoogVCPU *dbg_vcpu;
};
