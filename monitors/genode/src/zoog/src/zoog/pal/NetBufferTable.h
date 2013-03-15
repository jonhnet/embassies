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

#include <base/lock.h>
#include <zoog_monitor_session/connection.h>
#include "hash_table.h"
#include "linked_list.h"

using namespace ZoogMonitor;

namespace ZoogPal {

	class AllocatedBuffer {
	private:
		ZoogMonitor::Session::Zoog_genode_net_buffer_id _buffer_id;
		Dataspace_capability _buffer_cap;
		void *_guest_mapping;

	public:
		ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id();

		ZoogNetBuffer *znb();

		static uint32_t hash(const void *v_a);

		static int cmp(const void *v_a, const void *v_b);

		/* called when we know server has yoinked the mapping, so
		 * a detach would merely produce a "no attachment at 2000" warning.
		 */

		AllocatedBuffer(Dataspace_capability buffer_cap);
		
		AllocatedBuffer(ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id);

		~AllocatedBuffer();
	};

	class NetBufferTable {
	private:
		Lock _lock;
		HashTable _ht;

		ZoogMonitor::Session::Zoog_genode_net_buffer_id retrieve(ZoogNetBuffer *znb, AllocatedBuffer **out_buffer, const char *state);

		static void enumerate_znb_foreach(void *v_list, void *v_ab);

	public:
		NetBufferTable();

		ZoogNetBuffer *insert(Dataspace_capability buffer_cap);

		ZoogMonitor::Session::Zoog_genode_net_buffer_id lookup(ZoogNetBuffer *znb, const char *state);

		/* remove after send-with-release */
		void remove(ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id);

		/* remove before server has yoinked the segment back; free_net_buffer case */
		ZoogMonitor::Session::Zoog_genode_net_buffer_id remove(ZoogNetBuffer *znb);

		void enumerate_znbs(LinkedList *list);
	};
}
