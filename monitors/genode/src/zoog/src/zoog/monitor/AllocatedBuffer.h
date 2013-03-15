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

using namespace Genode;

namespace ZoogMonitor {
	
	class AllocatedBuffer {
	private:
		Session::Zoog_genode_net_buffer_id _buffer_id;
		uint32_t _allocated_payload_size;
		Ram_dataspace_capability _dataspace_cap;
		void *_host_mapping;

		AllocatedBuffer *retrieve_buffer(Session::Zoog_genode_net_buffer_id buffer_id);

	public:

		Dataspace_capability cap() { return _dataspace_cap; }

		uint32_t capacity() { return _allocated_payload_size; }

		Session::Zoog_genode_net_buffer_id buffer_id() {
			return ((Session::Zoog_genode_net_buffer_id *) _host_mapping)[0];
		}

		ZoogNetBuffer *znb() {
			return (ZoogNetBuffer *)
				&((Session::Zoog_genode_net_buffer_id *) _host_mapping)[1];
		}

		void *payload() {
			return &znb()[1];
		}

		static uint32_t hash(const void *v_a) {
			uint32_t result = ((AllocatedBuffer*)v_a)->_buffer_id;
//			PDBG("hashed %08x giving %d", v_a, result);
			return result;
		}

		static int cmp(const void *v_a, const void *v_b) {
			AllocatedBuffer *a = (AllocatedBuffer *) v_a;
			AllocatedBuffer *b = (AllocatedBuffer *) v_b;
			int result = (a->_buffer_id) - (b->_buffer_id);
//			PDBG("cmp %08x,%08x : %d, %d giving %d",
//				v_a, v_b, a->_buffer_id, b->_buffer_id, result);
			return result;
		}

		AllocatedBuffer(Session::Zoog_genode_net_buffer_id buffer_id, uint32_t payload_size) {
			_buffer_id = buffer_id;
			_dataspace_cap = env()->ram_session()->alloc(
				sizeof(Session::Zoog_genode_net_buffer_id)
				+ payload_size
				+ sizeof(ZoogNetBuffer));
			_host_mapping = env()->rm_session()->attach(_dataspace_cap);
			((Session::Zoog_genode_net_buffer_id *) _host_mapping)[0] = buffer_id;
			znb()->capacity = payload_size;
			_allocated_payload_size = payload_size;
		}

		// key ctor
		AllocatedBuffer(Session::Zoog_genode_net_buffer_id buffer_id) {
			_buffer_id = buffer_id;
			_host_mapping = NULL;
		}

		~AllocatedBuffer() {
			if (_host_mapping==NULL) {
				return;	// this is just a key
			}
			env()->rm_session()->detach(_host_mapping);
			env()->ram_session()->free(_dataspace_cap);
		}
	};
}
