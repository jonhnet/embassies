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
 * \brief  Waits for Genode signals from monitor; used to invoke debug services
 * \author Jon Howell
 * \date   2012.01.18
 */

#pragma once

#include <base/thread.h>
#include <base/signal.h>
#include <zoog_monitor_session/connection.h>

using namespace Genode;

class CoreDumpInvokeIfc {
public:
	virtual void dump_core() = 0;
};

class DebugServer
	: private Thread<32768>
{
private:
	CoreDumpInvokeIfc *_core_dump_invoke_ifc;
	Signal_context core_dump_invoke_context;
	Signal_receiver signal_receiver;

	void entry();

public:
	DebugServer(
		CoreDumpInvokeIfc *_core_dump_invoke_ifc,
		ZoogMonitor::Connection *zoog_monitor);
		
};
