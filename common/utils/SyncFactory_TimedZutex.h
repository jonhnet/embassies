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

#include "SyncFactory_Zutex.h"
#include "zutex_sync.h"
#include "ZClock.h"

class SyncFactoryEvent_TimedZutex : public SyncFactoryEvent_Zutex
{
public:
	virtual ~SyncFactoryEvent_TimedZutex() {}
	virtual bool wait(int timeout_ms);

private:
	friend class SyncFactory_TimedZutex;

	SyncFactoryEvent_TimedZutex(
		ZoogDispatchTable_v1 *zdt,
		bool auto_reset,
		ZClock *zclock);

	ZClock *zclock;
	ZTimer *ztimer;
};

class SyncFactory_TimedZutex : public SyncFactory_Zutex
{
public:
	SyncFactory_TimedZutex(ZoogDispatchTable_v1 *zdt, ZClock *zclock);
	virtual SyncFactoryEvent *new_event(bool auto_reset);

private:
	ZClock *zclock;
};


