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
}
