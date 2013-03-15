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

#include <ctype.h>
#include <pthread.h>
#include "linux_kvm_call_page.h"
#include "linux_kvm_protocol.h"
#include "ZutexWaiter.h"
#include "core_regs.h"
#include "MemSlot.h"
#include <linux/kvm.h>
#include "VCPUPool.h"
#include "NetBuffer.h"
#include "VCPUYieldControlIfc.h"

class ZoogVM;

class ZoogVCPU
	: public VCPUYieldControlIfc
{
public:
	ZoogVCPU(ZoogVM *vm, uint32_t guest_entry_point, uint32_t stack_top_guest);
		// used for thread_create, where app has already set up
		// its own stack.

	ZoogVCPU(ZoogVM *vm, uint32_t guest_entry_point);
		// allocate a minimal stack internally. Used for starting
		// bootstrap thread, which calls PAL to allocate further memory;
		// the PAL call requires a stack.

	~ZoogVCPU();

	void pause();
	void resume();
	void get_registers(Core_x86_registers *regs);

	int get_zid() { return zid; }
		// Just an identifier, used to label these things in debug output,
		// and in corefiles

	// VCPUYieldControlIfc
	virtual bool vcpu_held();
	virtual int vcpu_yield_timeout_ms();
	virtual void vcpu_yield();

private:
	VCPUAllocation *vcpu_allocation;
		// NULL if vcpu has been yielded back to pool
	// thread snapshot state for when the thread yields its vcpu
	// (Since only 16 kvm vcpus are available, we have to multiplex them.)
	struct kvm_regs snap_regs;
	struct kvm_sregs snap_sregs;

	int zid;
	ZoogVM *vm;
	uint32_t guest_entry_point;
	uint32_t stack_top_guest;
	pthread_t pt;
	bool thread_condemned;
	MemSlot *gdt_page;
	ZoogKvmCallPage *call_page;
	uint32_t call_page_capacity;
		// NB we keep a private copy of call_page->capacity, because
		// we perform bounds checks using it, and guest can write to
		// call_page->capacity, so it's not a trustworthy value.

	pid_t my_thread_id;
	pthread_mutex_t pause_mutex;
	pthread_cond_t done_pause_cond;
	pthread_mutex_t has_paused_mutex;
	pthread_cond_t has_paused_cond;
	bool pause_control_flag;
	bool is_paused_flag;

	ZutexWaiter *zutex_waiter;
		// The zutex_waiter object represents us when we're waiting
		// on ZutexQueues.

	struct gdt_table_entry {
		uint16_t limit_low;
		uint16_t base_low;
		uint8_t base_middle;
		uint8_t access;
		uint8_t granularity;
		uint8_t base_high;
	} *gdt_entries;
	void _init_segment(kvm_segment *seg, uint32_t base, int type, int s, int selector);
	void gdt_set_gate(uint32_t idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
	void kablooey(bool wait_patiently);

	static void *zvcpu_thread(void *obj);
	void _init(ZoogVM *vm, uint32_t guest_entry_point, uint32_t stack_top_guest);
	void run();
	void service_loop();
	void handle_zoogcall();
	void clean_string(char *dst_str, int capacity, const char *src_str);

	void allocate_memory(xa_allocate_memory *xa_guest);
	void free_memory(xa_free_memory *xa_guest);
	void thread_create(xa_thread_create *xa_guest);
	void thread_exit(xa_thread_exit *xa_guest);
	void x86_set_segments(xa_x86_set_segments *xa_guest);
	void exit(xa_exit *xa_guest);
	void launch_application(xa_launch_application *xa_guest);
	void endorse_me(xa_endorse_me *xa_guest);
	void get_app_secret(xa_get_app_secret *xa_guest);
	void get_random(xa_get_random *xa_guest);

	void zutex_wait(xa_zutex_wait *xa_guest);
	void zutex_wake(xa_zutex_wake *xa_guest);

	void get_ifconfig(xa_get_ifconfig *xa_guest);
	void alloc_net_buffer(xa_alloc_net_buffer *xa_guest);
	void send_net_buffer(xa_send_net_buffer *xa_guest);
	void _free_net_buffer(NetBuffer* nb);
	void receive_net_buffer(xa_receive_net_buffer *xa_guest);
	void free_net_buffer(xa_free_net_buffer *xa_guest);

	void get_time(xa_get_time *xa_guest);
	void set_clock_alarm(xa_set_clock_alarm *xa_guest);

	void extn_debug_logfile_append(xa_extn_debug_logfile_append *xa_guest);
	void extn_debug_create_toplevel_window(xa_extn_debug_create_toplevel_window *xa_guest);
	void unknown_zoogcall(XP_Opcode opcode);
	void unimpl(XP_Opcode opcode);
	void internal_find_app_code(xa_internal_find_app_code *xa_guest);
	void internal_map_alarms(xa_internal_map_alarms *xa_guest);


	void sublet_viewport(xa_sublet_viewport *xa_guest);
	void repossess_viewport(xa_repossess_viewport *xa_guest);
	void get_deed_key(xa_get_deed_key *xa_guest);
	void accept_viewport(xa_accept_viewport *xa_guest);
	void transfer_viewport(xa_transfer_viewport *xa_guest);
	void verify_label(xa_verify_label *xa_guest);
	void map_canvas(xa_map_canvas *xa_guest);
	void unmap_canvas(xa_unmap_canvas *xa_guest);
	void update_canvas(xa_update_canvas *xa_guest);
	void receive_ui_event(xa_receive_ui_event *xa_guest);

	void debug_set_idt_handler(xa_debug_set_idt_handler *xa_guest);

	void init_pause_controls();
	void free_pause_controls();
	static void sig_int_absorber(int signum);

	inline int vcpufd() { return vcpu_allocation->get_vcpufd(); }
	void profile_calls(XP_Opcode opcode);

	void claim_kvm_vcpu();
	void yield_kvm_vcpu();

};

