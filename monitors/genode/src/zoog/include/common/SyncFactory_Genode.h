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

#include <base/lock.h>
#include <base/semaphore.h>

#include "SyncFactory.h"
#include "Thread_id.h"

using namespace Genode;

class SyncFactoryMutex_Genode
	: public SyncFactoryMutex
{
private:
	Lock blocking;
	Lock fast;	// blocking must be held before taking fast
	int depth;
	Thread_id holder;

public:
	SyncFactoryMutex_Genode(bool recursive);
	virtual ~SyncFactoryMutex_Genode() {};
	virtual void lock();
	virtual void unlock();
};

class SyncFactoryEvent_Genode
	: public SyncFactoryEvent
{
private:
	Lock _lock;
	Semaphore _sem;
	bool _signaled;
	bool _auto_reset;

public:
	SyncFactoryEvent_Genode(bool auto_reset);
	virtual ~SyncFactoryEvent_Genode() {};
	virtual void wait();
	virtual bool wait(int timeout_ms);
	virtual void signal();
	virtual void reset();
};

class SyncFactory_Genode
	: public SyncFactory
{
public:
	virtual SyncFactoryMutex *new_mutex(bool recursive);
	virtual SyncFactoryEvent *new_event(bool auto_reset);
};

