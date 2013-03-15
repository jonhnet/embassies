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

// Protocol between monitor processes and coordinator.
// Not security-sensitive; we assume monitors have already
// done sanity checking and are producing well-formed messages.
// (That is, these message are from inside the TCB to inside the TCB.)

#include "pal_abi/pal_net.h"
#include "pal_abi/pal_ui.h"

#define ZOOG_KVM_RENDEZVOUS "/tmp/zoog_kvm_coordinator"
#define ZK_SMALL_MESSAGE_LIMIT	70000

typedef enum CoordinatorOpcode {
	co_connect = 0x200,
	co_connect_complete,
	co_long_message,
	co_free_long_message,
	co_deliver_packet,
	co_deliver_packet_long,
	co_launch_application,
	co_sublet_viewport,
	co_sublet_viewport_reply,
	co_repossess_viewport,
	co_repossess_viewport_reply,
	co_get_deed_key,
	co_get_deed_key_reply,
	co_accept_viewport,
	co_accept_viewport_reply,
	co_transfer_viewport,
	co_transfer_viewport_reply,
	co_verify_label,
	co_verify_label_reply,
	co_map_canvas,
	co_map_canvas_reply,
	co_unmap_canvas,
	co_unmap_canvas_reply,
	co_update_canvas,
	co_deliver_ui_event,
	co_extn_debug_create_toplevel_window,
	co_extn_debug_create_toplevel_window_reply,
	co_get_monitor_key_pair,
	co_get_monitor_key_pair_reply,
	co_internal_failure_code,
	co_alloc_long_message,
	co_alloc_long_message_reply,
} CoordinatorOpcode;

typedef struct {
	uint32_t length;	// including this header
	CoordinatorOpcode opcode;
} CMHeader;

typedef struct {
	CMHeader hdr;
	uint32_t pub_key_len;
	ZPubKey pub_key[0];
} CMConnect;

typedef struct {
	CMHeader hdr;
	int count;
	XIPifconfig ifconfigs[2];
} CMConnectComplete;

	typedef struct {
		char mmap_message_path[0];
	} CMMmapIdentifier;

	typedef struct {
		int key;
		uint32_t length;
	} CMShmIdentifier;

	typedef struct {
		enum { CL_MMAP, CL_SHM } type;
		union {
			CMMmapIdentifier mmap;
				// receiver may assume path includes a terminating 0
			CMShmIdentifier shm;
		} un;
	} CMLongIdentifier;

typedef struct {
	CMHeader hdr;
	CMLongIdentifier id;
} CMLongMessage;

typedef struct {
	CMHeader hdr;
	CMLongIdentifier id;
} CMFreeLongMessage;

typedef struct {
	CMHeader hdr;
	uint8_t payload[0];
} CMDeliverPacket;

typedef struct {
	CMHeader hdr;
//	uint8_t prefix[64];
		// a peek at header so coordinator can route the packet without
		// mapping it
	CMLongIdentifier id;
} CMDeliverPacketLong;

typedef struct {
	CMHeader hdr;
	uint8_t boot_block[0];
} CMLaunchApplication;

typedef struct {
	CMHeader hdr;
	uint32_t rpc_nonce;
} CMRPCHeader;

#if 0

typedef struct {
	CMHeader hdr;
	uint32_t rpc_nonce;
	ZCanvasID canvas_id;
	uint32_t pub_key_len;
	uint8_t pub_key[0];
} CMDelegateViewport;

typedef struct {
	CMHeader hdr;
	uint32_t rpc_nonce;
	ViewportID viewport_id;
} CMDelegateViewportReply;

#endif

typedef struct {
	enum { OPCODE = co_sublet_viewport };
	CMRPCHeader rpc;
	ViewportID in_tenant_viewport;
	ZRectangle in_rectangle;
} CMSubletViewport;

typedef struct {
	enum { OPCODE = co_sublet_viewport_reply };
	CMRPCHeader rpc;
	ViewportID out_landlord_viewport;
	Deed out_deed;
} CMSubletViewportReply;

typedef struct {
	enum { OPCODE = co_repossess_viewport };
	CMRPCHeader rpc;
	ViewportID in_landlord_viewport;
} CMRepossessViewport;

typedef struct {
	enum { OPCODE = co_repossess_viewport_reply };
	CMRPCHeader rpc;
} CMRepossessViewportReply;

typedef struct {
	enum { OPCODE = co_get_deed_key };
	CMRPCHeader rpc;
	ViewportID in_landlord_viewport;
} CMGetDeedKey;

typedef struct {
	enum { OPCODE = co_get_deed_key_reply };
	CMRPCHeader rpc;
	DeedKey out_deed_key;
} CMGetDeedKeyReply;

typedef struct {
	enum { OPCODE = co_accept_viewport };
	CMRPCHeader rpc;
	Deed in_deed;
} CMAcceptViewport;

typedef struct {
	enum { OPCODE = co_accept_viewport_reply };
	CMRPCHeader rpc;
	ViewportID out_tenant_viewport;
	DeedKey out_deed_key;
} CMAcceptViewportReply;

typedef struct {
	enum { OPCODE = co_transfer_viewport };
	CMRPCHeader rpc;
	ViewportID in_tenant_viewport;
} CMTransferViewport;

typedef struct {
	enum { OPCODE = co_transfer_viewport_reply };
	CMRPCHeader rpc;
	Deed out_deed;
} CMTransferViewportReply;

typedef struct {
	// NB semantics are that the chain is already verified by the
	// (trusted) monitor; this is just a synchronous call to tell
	// the coordinator to start allowing & labeling windows
	enum { OPCODE = co_verify_label };
	CMRPCHeader rpc;
} CMVerifyLabel;

typedef struct {
	enum { OPCODE = co_verify_label_reply };
	CMRPCHeader rpc;
} CMVerifyLabelReply;

typedef struct {
	enum { OPCODE = co_map_canvas };	// tricky
	CMRPCHeader rpc;
	ViewportID viewport_id;
	PixelFormat known_formats[4];
	int num_formats;
} CMMapCanvas;

typedef struct {
	enum { OPCODE = co_map_canvas_reply };
	CMRPCHeader rpc;
	ZCanvas canvas;
	CMLongIdentifier id;
} CMMapCanvasReply;

typedef struct {
	enum { OPCODE = co_unmap_canvas };
	CMRPCHeader rpc;
	ZCanvasID canvas_id;
} CMUnmapCanvas;

typedef struct {
	enum { OPCODE = co_unmap_canvas_reply };
	CMRPCHeader rpc;
} CMUnmapCanvasReply;

typedef struct {
	CMHeader hdr;
	ZCanvasID canvas_id;
	ZRectangle rectangle;
} CMUpdateCanvas;

typedef struct {
	CMHeader hdr;
	ZoogUIEvent event;
} CMDeliverUIEvent;

typedef struct {
	enum { OPCODE = co_extn_debug_create_toplevel_window };
	CMRPCHeader rpc;
} CMExtnDebugCreateToplevelWindow;

typedef struct {
	enum { OPCODE = co_extn_debug_create_toplevel_window_reply };
	CMRPCHeader rpc;
	ViewportID out_viewport_id;
} CMExtnDebugCreateToplevelWindowReply;

typedef struct {
	enum { OPCODE = co_get_monitor_key_pair };
	CMRPCHeader rpc;
} CMGetMonitorKeyPair;

typedef struct {
	enum { OPCODE = co_get_monitor_key_pair_reply };
	CMRPCHeader rpc;
	uint32_t monitor_key_pair_len;
	uint8_t monitor_key_pair[0];	// ZKeyPair
} CMGetMonitorKeyPairReply;

typedef struct {
	enum { OPCODE = co_alloc_long_message };
	CMRPCHeader rpc;
	uint32_t size;
} CMAllocLongMessage;
typedef struct {
	enum { OPCODE = co_alloc_long_message_reply };
	CMRPCHeader rpc;
	CMLongIdentifier id;
} CMAllocLongMessageReply;
