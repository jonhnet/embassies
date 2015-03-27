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
 * \brief  AllocatedBuffer - wraps mapping & casting of ZoogNetBuffer
 *     shared memory
 * \author Jon Howell
 * \date   2011-12-29
 */

#pragma once

#include <base/printf.h>
#include <base/env.h>
#include <base/sleep.h>
#include <cap_session/connection.h>
#include <root/component.h>
#include <zoog_monitor_session/zoog_monitor_session.h>
#include <base/rpc_server.h>
#include <rom_session/connection.h>

#include <nic_session/connection.h>

extern char _binary_paltest_raw_start;
extern char _binary_paltest_raw_end;

namespace ZoogMonitor {
	
	class AllocatedBuffer {
	private:
		Session::Zoog_genode_net_buffer_id _buffer_id;
		uint32_t _allocated_payload_size;
		Ram_dataspace_capability _dataspace_cap;
		void *_host_mapping;

	public:

		Dataspace_capability cap();

		uint32_t capacity();

		Session::Zoog_genode_net_buffer_id buffer_id();

		ZoogNetBuffer *znb();

		void *payload();

		static uint32_t hash(const void *v_a);
		static int cmp(const void *v_a, const void *v_b);

		AllocatedBuffer(Session::Zoog_genode_net_buffer_id buffer_id, uint32_t payload_size);
		// key ctor
		AllocatedBuffer(Session::Zoog_genode_net_buffer_id buffer_id);

		~AllocatedBuffer();
	};
}
