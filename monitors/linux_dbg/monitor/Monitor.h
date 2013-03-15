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

#include <sys/time.h>

#include "linked_list.h"
#include "netifc.h"
#include "Process.h"
#include "PacketQueue.h"
#include "ZKeyPair.h"
#include "hash_table.h"
#include "SyncFactory_Pthreads.h"

#include "ServiceThreadPool.h"
#include "Transaction.h"
#include "MonitorCrypto.h"
#include "TunIDAllocator.h"

//#define XNB_NOTE(e) 	// fprintf(stderr, e);
/* Uncomment to print zoog-call timings
#define XNB_NOTE(e) 	{ \
	struct timeval t; gettimeofday(&t,NULL); \
	fprintf(stderr, "%d.%03d %s", (int) t.tv_sec, (int) t.tv_usec/1000, e); }
*/
#define XNB_NOTE(e) ;

class Monitor {
private:
	int xax_port_socket;
	MallocFactory *mf;
	SyncFactory_Pthreads sf;
	ServiceThreadPool service_thread_pool;

	struct sockaddr_un xp_addr;
	
	TunIDAllocator tunid;
	NetIfc netifc;

	HashTable active_process_ht;
	HashTable idle_process_ht;
		// process has been recently touched by clock
	HashTable stale_process_ht;
		// process has been touched by clock twice, and hence is stale
//	Process *exalted_process;
		// the most-recently-created process. It can't be completely
		// culled, because it might just be being debugged.

	pthread_mutex_t mutex;

	PacketAccount *packet_account;

	MonitorCrypto crypto;

	BlitterManager *blitter_manager;

	bool dbg_allow_unlabeled_ui;

	enum { STALE_PROCESS_PACKET_THRESHHOLD = 30 };

	static void *static_run(void *v_m);
	void run();
	void bludgeon_zombies();
	static void *static_timer_loop(void *vm);
	void timer_loop();
	Process *_lookup_process_nolock(Process *key);
	Process *_extract_from_demoted_table(HashTable *ht, Process *key);
	void _disconnect_process_inner(Process *process);

	friend class DemoteSpec;

public:
	Monitor();

	MallocFactory *get_mf() { return mf; }
	SyncFactory *get_sf() { return &sf; }
	PacketAccount *get_packet_account() { return packet_account; }
	int get_xax_port_socket() { return xax_port_socket; }
	NetIfc *get_netifc() { return &netifc; }
	ZKeyPair *get_monitorKeyPair() { return crypto.get_monitorKeyPair(); }
	ZPubKey *get_zoog_ca_root_key() { return crypto.get_zoog_ca_root_key(); }
	KeyDerivationKey *get_app_master_key() { return crypto.get_app_master_key(); }
	void get_random_bytes(uint8_t *out_bytes, uint32_t length);
	BlitterManager *get_blitter_manager() { return blitter_manager; }

	void start_new_thread();
	Process *connect_process(uint32_t process_id, ZPubKey *pub_key);
	void disconnect_process(Process *process);
	Process *lookup_process(uint32_t process_id);
	bool get_dbg_allow_unlabeled_ui() { return dbg_allow_unlabeled_ui; }
	int get_tunid() { return tunid.get_tunid(); }
};
