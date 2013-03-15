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
 * \brief  Genode_pal implements Zoog PAL ABI on Genode
 * \author Jon Howell
 * \date   2011.12.24
 */

#pragma once

#include <base/thread.h>
#include <timer_session/connection.h>
#include "EventSynchronizerIfc.h"
#include "SyncFactory.h"

class ZutexTable;
class HostAlarms;

using namespace Genode;

class ZoogAlarmThread
	: private Thread<1024>
{
private:
	Timer::Connection timer;
	ZutexTable *zutex_table;
	EventSynchronizerIfc *event_synchronizer;
	SyncFactoryMutex *mutex;

	uint64_t curr_time;
	uint64_t next_due;
	uint32_t sequence_number;

	void entry();

public:
	ZoogAlarmThread(ZutexTable *zutex_table, EventSynchronizerIfc *event_synchronizer, SyncFactory *sf);
	void reset_alarm(uint64_t next_alarm);
	uint64_t get_time();
};
