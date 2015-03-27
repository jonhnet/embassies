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
 * \brief  Client-side interface of the ZoogMonitor service
 * \author Jon Howell
 * \date   2011-12-22
 */

#pragma once

#include <zoog_monitor_session/zoog_monitor_session.h>
#include <base/rpc_client.h>
#include <base/printf.h>

namespace ZoogMonitor {

	struct Session_client : Genode::Rpc_client<Session>
	{
		Session_client(Genode::Capability<Session> cap)
		: Genode::Rpc_client<Session>(cap) { }

		Dataspace_capability map_dispatch_region(uint32_t region_count) {
			return call<Rpc_map_dispatch_region>(region_count);
		}

		Dataspace_capability get_boot_block() {
			return call<Rpc_get_boot_block>();
		}

		Dataspace_capability alloc_net_buffer(uint32_t payload_size) {
			return call<Rpc_alloc_net_buffer>(payload_size);
		}

		Receive_net_buffer_reply receive_net_buffer() {
			return call<Rpc_receive_net_buffer>();
		}

		void event_receive_sighs(
			Signal_context_capability network_cap,
			Signal_context_capability ui_cap) {
			call<Rpc_event_receive_sighs>(network_cap, ui_cap);
		}
		
		void core_dump_invoke_sigh(Signal_context_capability cap) {
			call<Rpc_core_dump_invoke_sigh>(cap);
		}
		
		void start_gate_sigh(Signal_context_capability cap) {
			call<Rpc_start_gate_sigh>(cap);
		}
		
		MapCanvasReply map_canvas(ViewportID viewport_id) {
			return call<Rpc_map_canvas>(viewport_id);
		}

		void dispatch_message(uint32_t offset) {
			call<Rpc_dispatch_message>(offset);
		}

		////////////////////////////////////////////////////////////

	};
}
