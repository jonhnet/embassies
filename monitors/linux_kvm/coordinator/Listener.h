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

#include <sys/un.h>

#include "CoalescingAllocator.h"
#include "Router.h"
#include "Message.h"
#include "LongMessageAllocator.h"
#include "BlitterManager.h"
#include "hash_table.h"
#include "Table.h"
#include "Shm.h"
#include "MonitorCrypto.h"
#include "TunIDAllocator.h"

class TunnelAddress : public UserObjIfc
{
public:
	~TunnelAddress() {}
};

class Listener
{
private:
	MallocFactory *mf;
	SyncFactory *sf;
	int rendezvous_socket;
	struct sockaddr_un rendezvous_addr;
	CoalescingAllocator *tunnel_address_allocator;
	Router *router;
	LongMessageAllocator* long_message_allocator;
	BlitterManager *blitter_mgr;
	Table *sockaddr_to_app_table;
		// TODO lock this data structure
	TunIDAllocator tunid;
	Shm* shm;

	MonitorCrypto crypto;

	App *msg_to_app(Message *msg, CoordinatorOpcode opcode);
	void dispatch(Message *cm);
	void connect(Message *cm, CMConnect *conn);
	void launch_application(CMLaunchApplication *la);
	void free_long_message(App* app, CMFreeLongMessage * hdr);
	void deliver_packet(Message *msg);
	void deliver_packet_long(CMDeliverPacketLong* dpl);
	BlitterManager* blitterMgr() { return blitter_mgr; }
	void sublet_viewport(App* app, CMSubletViewport* hdr);
	void repossess_viewport(App* app, CMRepossessViewport* hdr);
	void get_deed_key(App* app, CMGetDeedKey* hdr);
	void accept_viewport(App* app, CMAcceptViewport* hdr);
	void transfer_viewport(App* app, CMTransferViewport* hdr);
	void verify_label(App* app, CMVerifyLabel* hdr);
	void map_canvas(App* app, CMMapCanvas * hdr);
	void unmap_canvas(App* app, CMUnmapCanvas * hdr);
	void update_canvas(CMUpdateCanvas *hdr);
	void extn_debug_create_toplevel_window(App* app, CMExtnDebugCreateToplevelWindow * hdr);
	void get_monitor_key_pair(App* app, CMGetMonitorKeyPair* hdr);
	void alloc_long_message(App* app, CMAllocLongMessage* hdr);

public:
	Listener(MallocFactory *mf);
	void run();

	// hook used by App::send_cm
	int get_rendezvous_socket() { return rendezvous_socket; }
	Router *get_router() { return router; }
	MallocFactory *get_malloc_factory() { return mf; };
	LongMessageAllocator *get_long_message_allocator()
		{ return long_message_allocator; }

	void disconnect(App *app);
	Shm *get_shm() { return shm; }
};
