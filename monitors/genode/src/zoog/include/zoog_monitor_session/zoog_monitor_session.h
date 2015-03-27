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
 * \brief  Interface definition for ZoogMonitor service
 * \author Jon Howell
 * \date   2011-12-22
 */

#pragma once

#include <session/session.h>
#include <base/rpc.h>
#include <base/signal.h>
#include <dataspace/capability.h>

#include "pal_abi/pal_net.h"
#include "pal_abi/pal_ui.h"

using namespace Genode;
namespace ZoogMonitor {

	struct Session : Genode::Session
	{
		static const char *service_name() { return "ZoogMonitor"; }

		typedef uint32_t Zoog_genode_net_buffer_id;
		enum { NUM_IFCONFIGS = 2 };
		struct Ifconfig_reply {
			uint32_t num_ifconfigs;
			XIPifconfig ifconfigs[NUM_IFCONFIGS];
		};
		struct Receive_net_buffer_reply {
			bool valid;	// false if no packets ready
			Dataspace_capability cap;
		};
		struct MapCanvasReply {
			ZCanvas canvas;
			Dataspace_capability framebuffer_dataspace_cap;
		};

		
		virtual Dataspace_capability map_dispatch_region(uint32_t region_count) = 0;
		virtual Dataspace_capability get_boot_block() = 0;

		virtual Dataspace_capability alloc_net_buffer(uint32_t payload_size) = 0;
		virtual Receive_net_buffer_reply receive_net_buffer() = 0;
		virtual void event_receive_sighs(
			Signal_context_capability network_cap,
			Signal_context_capability ui_cap) = 0;
		virtual void core_dump_invoke_sigh(Signal_context_capability cap) = 0;
		virtual void start_gate_sigh(Signal_context_capability start_gate_cap) = 0;
		virtual MapCanvasReply map_canvas(ViewportID viewport_id) = 0;

		struct DispatchGetIfconfig {
			struct {
				uint32_t num_ifconfigs;
				XIPifconfig ifconfigs[NUM_IFCONFIGS];
			} out;
		};
		struct DispatchSendNetBuffer {
			struct {
				Zoog_genode_net_buffer_id buffer_id;
				bool release;
			} in;
		};
		struct DispatchFreeNetBuffer {
			struct {
				Zoog_genode_net_buffer_id buffer_id;
			} in;
		};
		struct DispatchUnmapCanvas {
			struct {
				ZCanvasID canvas_id;
			} in;
		};
		struct DispatchUpdateCanvas {
			struct {
				ZCanvasID canvas_id;
				ZRectangle rect;
			} in;
		};
		struct DispatchReceiveUIEvent {
			struct {
				ZoogUIEvent evt;
			} out;
		};
		struct DispatchNewToplevelViewport {
			struct {
				ViewportID viewport_id;
			} out;
		};
		struct DispatchVerifyLabel {
			struct {
				uint32_t buffer_size;
				Zoog_genode_net_buffer_id buffer_id;
			} in;
			struct {
				bool result;
			} out;
		};

		struct DispatchSubletViewport {
			struct {
				ViewportID tenant_viewport;
				ZRectangle rectangle;
			} in;
			struct {
				ViewportID landlord_viewport;
				Deed deed;
			} out;
		};

		struct DispatchAcceptViewport {
			struct {
				Deed deed;
			} in;
			struct {
				ViewportID tenant_viewport;
				DeedKey deed_key;
			} out;
		};

		struct DispatchRepossessViewport {
			struct {
				ViewportID landlord_viewport;
			} in;
		};
				
		struct DispatchGetDeedKey {
			struct {
				ViewportID landlord_viewport;
			} in;
			struct {
				DeedKey deed_key;
			} out;
		};
				
		struct DispatchTransferViewport {
			struct {
				ViewportID tenant_viewport;
			} in;
			struct {
				Deed deed;
			} out;
		};

		struct DispatchLaunchApplication {
			struct {
				uint32_t buffer_size;
				Zoog_genode_net_buffer_id buffer_id;
			} in;
		};

		struct DispatchDebugWriteChannelOpen {
			struct {
				Zoog_genode_net_buffer_id buffer_id;
				uint32_t channel_name_size;	// string in the net_buffer
				uint32_t data_size;			// data coming in pieces via Write
			} in;
		};

		struct DispatchDebugWriteChannelWrite {
			struct {
				Zoog_genode_net_buffer_id buffer_id;
				uint32_t buffer_size;
			} in;
		};

		struct DispatchDebugWriteChannelClose {
		};

		struct DispatchAllocateMemory {
			struct {
				uint32_t length;
			} in;
			// uses out_dataspace_capability.
		};

		enum DispatchOp {
			do_get_ifconfig,
			do_send_net_buffer,
			do_free_net_buffer,
			do_unmap_canvas,
			do_update_canvas,
			do_receive_ui_event,
			do_new_toplevel_viewport,
			do_verify_label,
			do_sublet_viewport,
			do_accept_viewport,
			do_repossess_viewport,
			do_get_deed_key,
			do_transfer_viewport,
			do_launch_application,
			do_allocate_memory,

			do_debug_write_channel_open,
			do_debug_write_channel_write,
			do_debug_write_channel_close,
		};
		struct Dispatch {
			DispatchOp opcode;
			union {
				DispatchGetIfconfig get_ifconfig;
				DispatchSendNetBuffer send_net_buffer;
				DispatchFreeNetBuffer free_net_buffer;
				DispatchUnmapCanvas unmap_canvas;
				DispatchUpdateCanvas update_canvas;
				DispatchReceiveUIEvent receive_ui_event;
				DispatchNewToplevelViewport new_toplevel_viewport;
				DispatchVerifyLabel verify_label;
				DispatchSubletViewport sublet_viewport;
				DispatchAcceptViewport accept_viewport;
				DispatchRepossessViewport repossess_viewport;
				DispatchGetDeedKey get_deed_key;
				DispatchTransferViewport transfer_viewport;
				DispatchLaunchApplication launch_application;
				DispatchAllocateMemory allocate_memory;

				DispatchDebugWriteChannelOpen debug_write_channel_open;
				DispatchDebugWriteChannelWrite debug_write_channel_write;
				DispatchDebugWriteChannelClose debug_write_channel_close;
			} un;
			Dataspace_capability out_dataspace_capability;
		};

		virtual void dispatch_message(uint32_t index) = 0;

		/*******************
		 ** RPC interface **
		 *******************/

		GENODE_RPC(Rpc_map_dispatch_region, Dataspace_capability, map_dispatch_region, uint32_t);
		GENODE_RPC(Rpc_get_boot_block, Dataspace_capability, get_boot_block);

		GENODE_RPC(Rpc_alloc_net_buffer, Dataspace_capability, alloc_net_buffer, uint32_t);
		GENODE_RPC(Rpc_receive_net_buffer, Receive_net_buffer_reply, receive_net_buffer);
		GENODE_RPC(Rpc_event_receive_sighs, void, event_receive_sighs, Signal_context_capability, Signal_context_capability);
		GENODE_RPC(Rpc_core_dump_invoke_sigh, void, core_dump_invoke_sigh, Signal_context_capability);
		GENODE_RPC(Rpc_start_gate_sigh, void, start_gate_sigh, Signal_context_capability);
		GENODE_RPC(Rpc_map_canvas, MapCanvasReply, map_canvas, ViewportID);
		GENODE_RPC(Rpc_dispatch_message, void, dispatch_message, uint32_t);

		GENODE_RPC_INTERFACE(
			Rpc_map_dispatch_region,
			Rpc_get_boot_block,
			Rpc_alloc_net_buffer,
			Rpc_receive_net_buffer,
			Rpc_event_receive_sighs,
			Rpc_core_dump_invoke_sigh,
			Rpc_start_gate_sigh,
			Rpc_map_canvas,
			Rpc_dispatch_message
			);
	};
}
