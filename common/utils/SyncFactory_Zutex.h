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

#include "SyncFactory.h"
#include "zutex_sync.h"

class SyncFactoryMutex_Zutex : public SyncFactoryMutex
{
public:
	virtual ~SyncFactoryMutex_Zutex();
	virtual void lock();
	virtual void unlock();

protected:
	friend class SyncFactory_Zutex;
	SyncFactoryMutex_Zutex(ZoogDispatchTable_v1 *zdt);

	ZoogDispatchTable_v1 *zdt;
	ZMutex zmutex;
};

// The worst possible implementation of recursive mutices:
// a lock to check the owner, and then another lock for the
// actual mutex. Sorry! Not really exploiting the futex here, are we?
//
// recursive zutexes depend on %gs:(0) being available and
// a stable thread id indicator. I think these assumptions are
// true on posix.
class SyncFactoryRecursiveMutex_Zutex : public SyncFactoryMutex
{
public:
	virtual ~SyncFactoryRecursiveMutex_Zutex();
	virtual void lock();
	virtual void unlock();

protected:
	ZoogDispatchTable_v1 *zdt;
	ZMutex owner_id_zmutex;
	uint32_t owner_id;
	uint32_t depth;
	ZMutex main_zmutex;

	friend class SyncFactory_Zutex;
	SyncFactoryRecursiveMutex_Zutex(ZoogDispatchTable_v1 *zdt);
};

class SyncFactoryEvent_Zutex : public SyncFactoryEvent
{
public:
	virtual ~SyncFactoryEvent_Zutex();
	virtual void wait();
	virtual bool wait(int timeout_ms);
	virtual void signal();
	virtual void reset();

protected:
	friend class SyncFactory_Zutex;
	SyncFactoryEvent_Zutex(
		ZoogDispatchTable_v1 *zdt,
		bool auto_reset);

	ZoogDispatchTable_v1 *zdt;
	ZEvent zevent;
};

class SyncFactory_Zutex : public SyncFactory
{
public:
	SyncFactory_Zutex(ZoogDispatchTable_v1 *zdt);
		// if opt_zclock is supplied, the timeout version of event wait
		// will be available; otherwise, it will be asserted out.
	virtual SyncFactoryMutex *new_mutex(bool recursive);
	virtual SyncFactoryEvent *new_event(bool auto_reset);

protected:
	ZoogDispatchTable_v1 *zdt;
};


