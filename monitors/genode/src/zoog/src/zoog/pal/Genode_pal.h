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
#include "Dispatcher.h"

class CoreDumpTimer;

#if 0
class CtorDbg1 { public: CtorDbg1() { PDBG("CtorDbg1\n"); } };
class CtorDbg2 { public: CtorDbg2() { PDBG("CtorDbg2\n"); } };
class CtorDbg3 { public: CtorDbg3() { PDBG("CtorDbg3\n"); } };
class CtorDbg4 { public: CtorDbg4() { PDBG("CtorDbg4\n"); } };

class DbgLock {
private: Lock _lock;
public: 
	void lock() { PDBG("lock; this 0x%08x\n", (int) this); _lock.lock(); }
	void unlock() { PDBG("unlock\n"); _lock.unlock(); }
};
#endif

class GenodePAL
	: public CoreDumpInvokeIfc
{
private:
	Timer::Connection timer;
	ZoogMonitor::Connection zoog_monitor;
	ZoogDispatchTable_v1 zdt;

	enum { MAX_DISPATCH_SLOTS = 16 };
	bool dispatch_slot_used[MAX_DISPATCH_SLOTS];
	ZoogMonitor::Session::Dispatch *dispatch_region;

	SyncFactory_Genode sync_factory;
	CoalescingAllocator mem_allocator;
	UserObjIfc dummy_user_obj;
	MallocFactory *mf;
	Lock lock;

	ThreadTable thread_table;
	ZutexTable zutex_table;
	HostAlarms host_alarms;
	ZoogAlarmThread zoog_alarm_thread;
	MonitorAlarmThread monitor_alarm_thread;
	ZoogThread main_thread;
	ZoogPal::NetBufferTable net_buffer_table;
	uint8_t *boot_block;
	DebugServer debug_server;

	CanvasDownsamplerManager canvas_downsampler_manager;

	// core dump stuff
	LinkedList extra_ranges;

	CoreDumpTimer *core_dump_timer;
		// timer is deprecated now that we have a control interface
		// via port 1111

	uint32_t next_allocation_location;	/* avoids low-memory allocations */

	// multiplexed bulk disk I/O interface
	ChannelWriterClient *channel_writer;

	//////////////////////////////////////////////////////////////////////////

	enum { MIN_ALLOCATION_UNIT = 4<<20 };	/* 4MB */

	void idle();
	void launch(uint8_t *eip, uint8_t *esp, ZoogDispatchTable_v1 *zdt);
	void _grow_memory(size_t length);

	class PalLocker {
	private:
		GenodePAL *pal;
		int line;
		ZoogThread *t;
	public:
		PalLocker(GenodePAL *pal, int line);
		~PalLocker();
		ZoogThread *get_thread() { return t; }
	};
	friend class PalLocker;

	// must be used with pal locked.
	friend class Dispatcher;
	ZoogMonitor::Session::Dispatch* assign_dispatch_slot(int* out_idx);

	static void debug_logfile_append(const char *logfile_name, const char *message);
	static uint32_t debug_get_link_mtu();
	static ViewportID debug_create_toplevel_window();
	static void *lookup_extension(const char *name);
	static void *allocate_memory(size_t length);
	static void free_memory(void *base, size_t length);
	static bool thread_create(zoog_thread_start_f *func, void *stack);
	static void thread_exit(void);
	static void x86_set_segments(uint32_t fs, uint32_t gs);
	static void launch_application(SignedBinary *signed_binary);
	static void zutex_wait(ZutexWaitSpec *specs, uint32_t count);
	static int zutex_wake(
		uint32_t *zutex,
		uint32_t match_val, 
		Zutex_count n_wake,
		Zutex_count n_requeue,
		uint32_t *requeue_zutex,
		uint32_t requeue_match_val);
	static ZoogHostAlarms *get_alarms();
	static void get_ifconfig(XIPifconfig *ifconfig, int *inout_count);
	static ZoogNetBuffer *alloc_net_buffer(uint32_t payload_size);
	static void send_net_buffer(ZoogNetBuffer *znb, bool release);
	static ZoogNetBuffer *receive_net_buffer();
	static void free_net_buffer(ZoogNetBuffer *znb);
	static void get_time(uint64_t *out_time);
	static void set_clock_alarm(uint64_t scheduled_time);

// pal_ui.h
#if 0
	// Don't need these immediately.
	// landlord calls
	zf_zoog_repossess_viewport	zoog_repossess_viewport;
	zf_zoog_get_deed_key		zoog_get_deed_key;
	// tenant calls
	zf_zoog_transfer_viewport	zoog_transfer_viewport;
	static bool verify_label(ZCertChain* chain);
		// can stub this one out, too.
#endif
	static void sublet_viewport(
		ViewportID tenant_viewport,
		ZRectangle *rectangle,
		ViewportID *out_landlord_viewport,
		Deed *out_deed);

	static void accept_viewport(
		Deed *deed,
		ViewportID *out_tenant_viewport,
		DeedKey *out_deed_key);

	static void repossess_viewport(
		ViewportID landlord_viewport);

	static void get_deed_key(
		ViewportID landlord_viewport,
		DeedKey *out_deed_key);

	static void transfer_viewport(
		ViewportID tenant_viewport,
		Deed *out_deed);

	static bool verify_label(ZCertChain* chain);
	static void map_canvas(
		ViewportID viewport_id, PixelFormat *known_formats, int num_formats,
		ZCanvas *out_canvas);
	static void unmap_canvas(ZCanvasID canvas_id);
	static void update_canvas(ZCanvasID canvas_id, ZRectangle *rectangle);
	static void receive_ui_event(ZoogUIEvent *out_event);

	static void unimpl(void);

public:
	GenodePAL();
	void do_timer_thread();
	void dump_core();
};

