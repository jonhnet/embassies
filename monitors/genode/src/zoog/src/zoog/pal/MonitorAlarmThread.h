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
 * \brief  Waits for Genode signals from monitor; used to update monitor-driven HostAlarms
 * \author Jon Howell
 * \date   2011.12.24
 */

#pragma once

#include <base/thread.h>
#include <base/signal.h>
#include <zoog_monitor_session/connection.h>
#include "EventSynchronizerIfc.h"

class ZutexTable;
class HostAlarms;

using namespace Genode;

class MonitorAlarmThread
	: private Thread<1024>
{
private:
	EventSynchronizerIfc *event_synchronizer;
	uint32_t network_receive_sequence_number;
	uint32_t ui_event_sequence_number;

	Signal_context network_receive_context;
	Signal_context ui_event_receive_context;
	Signal_receiver signal_receiver;

	void entry();

public:
	MonitorAlarmThread(
		EventSynchronizerIfc *event_synchronizer,
		ZoogMonitor::Connection *zoog_monitor);
	void reset_alarm(uint64_t next_alarm);
	uint64_t get_time();
};
