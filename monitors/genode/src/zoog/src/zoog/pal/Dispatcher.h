/*
 * \brief  GenodePAL: genode-specific PAL for Zoog ABI
 * \author Jon Howell
 * \date   2011-12-25
 */

#pragma once

#include <base/env.h>
#include <base/lock.h>
#include <base/printf.h>
#include <zoog_monitor_session/client.h>
#include <timer_session/connection.h>
#include <zoog_monitor_session/connection.h>
#include <dataspace/client.h>
#include <base/sleep.h>

#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"

#include "common/SyncFactory_Genode.h"
#include "CoalescingAllocator.h"
#include "ThreadTable.h"
#include "ZutexWaiter.h"
#include "ZoogAlarmThread.h"
#include "MonitorAlarmThread.h"
#include "HostAlarms.h"
#include "NetBufferTable.h"
#include "DebugServer.h"
#include "ChannelWriterClient.h"
#include "CanvasDownsampler.h"
	
class Dispatcher {
private:
	GenodePAL* pal;
	int idx;
	ZoogMonitor::Session::Dispatch *dispatch;
public:
	Dispatcher(GenodePAL* pal, ZoogMonitor::Session::DispatchOp opcode);
	~Dispatcher();
	inline ZoogMonitor::Session::Dispatch* d() { return dispatch; }
	void rpc();
};

