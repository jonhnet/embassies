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
#include "linked_list.h"
#include "Packet.h"
#include "NetBuffer.h"
#include "CoordinatorProtocol.h"
#include "SyncFactory.h"
#include "UIEventQueue.h"
#include "CanvasAcceptor.h"
#include "LongMessageAllocator.h"
#include "linux_kvm_protocol.h"
#include "RPCToken.h"
#include "ZKeyPair.h"
#include "TunIDAllocator.h"

class CoordinatorConnection
{
private:
	LongMessageAllocator long_message_allocator;
	SyncFactory *sf;

	struct sockaddr_un localaddr;
	XIPifconfig ifconfigs[2];

	int tunid;
	int sock;

	pthread_mutex_t mutex;
	pthread_cond_t cond;
	uint32_t count;
	uint32_t size_bytes;
	LinkedList queue;

	UIEventQueue *uieq;
	HashTable rpc_table;
	uint32_t next_nonce;

	pthread_t recv_thread;
	bool connect_complete_recvd;	// assert we don't receive it twice
	SyncFactoryEvent *ifconfigs_ready;

	// queue-length limits
	uint32_t max_bytes;
	uint32_t min_packets;

private:
	void _send(void *data, uint32_t len);
	void _send_long_message(CMHeader *header, uint32_t header_len, void *data, uint32_t data_len);
	static void *_recv_thread(void *v_this);
	void dispatch_message(CMHeader *header);
	void decode_long_message(CMLongMessage* lm);
	void _recv_loop();
	void connect_complete(CMConnectComplete *cc);
	void deliver_packet(uint32_t cm_size, CMDeliverPacket *buf);
	void deliver_packet_long(uint32_t cm_size, CMDeliverPacketLong* dpl);
	void _enqueue_packet(Packet *p);
	Packet *_dequeue_nolock();
	void _tidy_nolock();
//	void accept_canvas_reply(CMAcceptCanvasReply *acr);
//	void delegate_viewport_reply(CMDelegateViewportReply *dvr);
//	void deliver_verify_label_reply();
	void deliver_ui_event(CMDeliverUIEvent *du);

	RPCToken* rpc_start(
		CMRPCHeader *req,
		CoordinatorOpcode req_opcode,
		uint32_t req_len,
		CMRPCHeader *reply,
		CoordinatorOpcode reply_opcode,
		uint32_t reply_len);

	void accept_rpc_reply(CMRPCHeader *rpc);

	friend class RPCToken;
	SyncFactory *_get_sf();	// for RPCToken

public:
	CoordinatorConnection(
		MallocFactory *mf,
		SyncFactory *sf,
		TunIDAllocator *tunid,
		EventSynchronizerIfc *event_synchronizer);
	~CoordinatorConnection();

	void connect(ZPubKey *pub_key);
	XIPifconfig *get_ifconfigs(int *out_count);
	void send_packet(NetBuffer *nb);
	Packet *dequeue_packet();
	void send_launch_application(void *image, uint32_t image_len);
	void send_extn_debug_create_toplevel_window(xa_extn_debug_create_toplevel_window *xa);
	ZKeyPair* send_get_monitor_key_pair();

	void send_free_long_message(LongMessageAllocation* lma);
	void send_sublet_viewport(xa_sublet_viewport *xa);
	void send_repossess_viewport(xa_repossess_viewport *xa);
	void send_get_deed_key(xa_get_deed_key *xa);
	void send_accept_viewport(xa_accept_viewport *xa);
	void send_transfer_viewport(xa_transfer_viewport *xa);
	void send_verify_label();
	void send_map_canvas(CanvasAcceptor *ca);
	void send_unmap_canvas(xa_unmap_canvas *xa);
	LongMessageAllocation *send_alloc_long_message(uint32_t size);

	void send_update_canvas(xa_update_canvas *xa);
	void dequeue_ui_event(ZoogUIEvent *out_event);
};
