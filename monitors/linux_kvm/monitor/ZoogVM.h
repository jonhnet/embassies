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

#include <stdint.h>
#include "malloc_factory.h"
#include "SyncFactory.h"
#include "linked_list.h"
#include "SmallIntegerAllocator.h"
	// can probably replace this with a CoalescingAllocator used in
	// trivial mode (ranges of size 1), now that it exists.
#include "CoalescingAllocator.h"
#include "NetBufferTable.h"
#include "CoordinatorConnection.h"
#include "ZutexTable.h"
#include "AlarmThread.h"
#include "MemoryMapIfc.h"
#include "MmapOverride.h"
#include "HostAlarms.h"
#include "ZKeyPair.h"
#include "KeyDerivationKey.h"
#include "MonitorCrypto.h"
#include "CallCounts.h"

class MemSlot;
class ZoogVCPU;
class VCPUPool;

class ZoogVM : public MemoryMapIfc {
public:
	ZoogVM(MallocFactory *mf, MmapOverride *mmapOverride, bool wait_for_core);

	uint32_t map_image(uint8_t *image, uint32_t size, const char *dbg_label);
		// returns guest address of loaded section
	void map_app_code(uint8_t *image, uint32_t size, const char *dbg_bootblock_path);
		// dbg_bootblock_path is written into any corefile we generate
	void set_pub_key(ZPubKey *pub_key);
	ZPubKey *get_pub_key();

	ZKeyPair *get_monitorKeyPair();
		// TCB's identity; used for endorsements
	KeyDerivationKey* get_app_master_key();
		// master symmetric secret used to generate per-app secrets
		// for zoog_get_app_secret().
	ZPubKey *get_zoog_ca_root_key();
		// CA root for verify_label

	void set_guest_entry_point(uint32_t entry_point_guest);
	void start();

	MemSlot *allocate_guest_memory(uint32_t size, const char *dbg_label);
	void free_guest_memory(MemSlot *mem_slot);
	void free_guest_memory(uint32_t guest_start, uint32_t size);

	// MemoryMapIfc
	void lock_memory_map();
	void unlock_memory_map();
	void *map_region_to_host(uint32_t guest_start, uint32_t size);

	void dbg_dump_memory_map();

	// accessors for ZoogVCPU.
	SyncFactory *get_sync_factory() { return sf; }
	int get_vmfd()
		{ return vmfd; }
	int get_kvmfd()
		{ return kvmfd; }
	void *get_guest_app_code_start()
		{ return guest_app_code_start; }
	uint32_t get_alarms_guest_addr(AlarmEnum alarm)
		{ return host_alarms->get_guest_addr(alarm); }
	RandomSupply *get_random_supply()
		{ return crypto.get_random_supply(); }

	VCPUPool *get_vcpu_pool()
		{ return vcpu_pool; }
	void record_vcpu(ZoogVCPU *vcpu);
	void remove_vcpu(ZoogVCPU *vcpu);
	NetBufferTable *get_net_buffer_table()
		{ return net_buffer_table; }
		// TODO needs mutex sync -- need to think about interactions
		// in send/recv queues.
	ZutexTable *get_zutex_table() { return zutex_table; }
	AlarmThread *get_alarm_thread() { return alarm_thread; }

	CoordinatorConnection *get_coordinator()
		{ return coordinator; }

	uint32_t get_idt_guest_addr() { return idt_page->get_guest_addr(); }
	void set_idt_handler(uint32_t target_address);

	// accessor for NetBufferTable
	MallocFactory *get_malloc_factory()
		{ return mf; }

	void emit_corefile(FILE *fp);

	void destroy();

	int allocate_zid() { return next_zid++; }
	void request_coredump();
	void request_coredump_immediate();
	CallCounts* get_call_counts() { return &call_counts; }
	bool avx_available() { return _avx_available; }

#if DBG_SEND_FAILURE
	FILE *dbg_send_log_fp;
#endif // DBG_SEND_FAILURE

private:
	MallocFactory *mf;
	int kvmfd;
	int vmfd;
	SyncFactory *sf;
	MmapOverride *mmapOverride;
	bool wait_for_core;
	bool _avx_available;

	SyncFactoryMutex *mutex;
	ZoogVCPU *root_vcpu;
	int next_zid;

	ZPubKey *pub_key;
	uint32_t entry_point_guest;
	bool coredump_request_flag;
	bool coredump_request_immediate;

	SyncFactoryMutex *memory_map_mutex;
		// protects guest_memory_allocator.
		// may be held longer during launch_application. I guess. Maybe stupid;
		// just reduce to mutex?
		// lock-order: take after this->mutex
	CoalescingAllocator *guest_memory_allocator;
	NetBufferTable *net_buffer_table;
	ZutexTable *zutex_table;
	MemSlot *host_alarms_page;
	HostAlarms *host_alarms;
	AlarmThread *alarm_thread;
	CoordinatorConnection *coordinator;

	MemSlot *idt_page;

	LinkedList vcpus;
	VCPUPool *vcpu_pool;

	void *guest_app_code_start;

	uint8_t *host_phys_memory;
	static const uint32_t host_phys_memory_size = 3<<29;	// 1.5GB

	char *dbg_bootblock_path;

	MonitorCrypto crypto;

	CallCounts call_counts;

	// private functions

	void _setup();
	void _map_physical_memory();
	void _pause_all();
	void _resume_all();
	void _test_cpu_flags();
};

