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

#include "linux_kvm_call_page.h"
#include "pal_abi/pal_abi.h"
#include "zoog_malloc_factory.h"

class KvmPal
{
private:
	ZoogDispatchTable_v1 zdt;
	static KvmPal *g_pal;
	ZoogHostAlarms *alarms;
	ZoogMallocFactory zmf;

	void _set_up_idt();
	void _map_alarms();
	static ZoogKvmCallPage *get_call_page();
	static void unimpl(const char *m);

	static void *lookup_extension(const char *name);
	static void *allocate_memory(size_t length);
	static void free_memory(void *base, size_t length);
	static bool thread_create(zoog_thread_start_f *func, void *stack);
	static void thread_exit();
	static void x86_set_segments(uint32_t fs, uint32_t gs);
	static void exit();
	static void launch_application(
		SignedBinary *signed_binary);

	static void endorse_me(
		ZPubKey* local_key, uint32_t cert_buffer_len, uint8_t* cert_buffer);
	static uint32_t get_app_secret(
	  uint32_t num_bytes_requested, uint8_t* buffer);
	static uint32_t get_random(uint32_t num_bytes_requested, uint8_t *buffer);
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

	static void get_time(
		uint64_t *out_time);
	static void set_clock_alarm(
		uint64_t in_time);


	static void sublet_viewport(
		ViewportID tenant_viewport,
		ZRectangle *rectangle,
		ViewportID *out_landlord_viewport,
		Deed *out_deed);

	static void repossess_viewport(
		ViewportID landlord_viewport);

	static void get_deed_key(
		ViewportID landlord_viewport,
		DeedKey *out_deed_key);

	static void accept_viewport(
		Deed *deed,
		ViewportID *out_tenant_viewport,
		DeedKey *out_deed_key);

	static void transfer_viewport(
		ViewportID tenant_viewport,
		Deed *out_deed);

	static bool verify_label(ZCertChain* chain);

	static void map_canvas(
		ViewportID viewport_id,
		PixelFormat *known_formats,
		int num_formats,
		ZCanvas *out_canvas);

	static void unmap_canvas(ZCanvasID canvas_id);

	static void update_canvas(ZCanvasID canvas_id,
		ZRectangle *rectangle);

	static void receive_ui_event(ZoogUIEvent *out_event);


	static void debug_logfile_append(const char *logfile_name, const char *message);
	static ViewportID debug_create_toplevel_window();
	static void idt_handler();

	static inline void call_monitor();

public:
	KvmPal();

	inline ZoogDispatchTable_v1 *get_dispatch_table() { return &zdt; }
	void *get_app_code_start();

};
