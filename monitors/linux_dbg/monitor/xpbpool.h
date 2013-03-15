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

#include "linux_dbg_port_protocol.h"
#include "Shm.h"
#include "hash_table.h"

class XPBPoolElt {
public:
	uint32_t identity;
	bool mapped_in_client;
	bool owned_by_client;
	bool frozen_for_send;
	BufferReplyType map_type;
	MapSpec map_spec; // to tell client what to map
	XaxPortBuffer *xpb;
	uint32_t mapping_size;	// so we can munmap() later

public:
	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);

	XPBPoolElt(XaxPortBuffer *xpb, uint32_t mapping_size, BufferReplyType map_type, MapSpec *map_spec);
	XPBPoolElt(uint32_t identity);	// key ctor
	~XPBPoolElt();

	bool busy();
	void freeze();
	void unfreeze();
	void sync_dbg();
};

class XPBPool {
private:
	HashTable ht;
	uint32_t name_counter;
	pthread_mutex_t mutex;
	Shm shm;

	static void _free_slot(void *v_this, void *v_obj);
	XPBPoolElt *_find_available(uint32_t payload_capacity);
	XPBPoolElt *_create_slot(uint32_t payload_capacity);
	XPBPoolElt *_lookup_elt_nolock(uint32_t handle);

public:
	XPBPool(MallocFactory *mf, int tunid);
	~XPBPool();

	XaxPortBuffer *get_buffer(uint32_t payload_capacity, XpBufferReply *buffer_reply);
	ZoogNetBuffer *get_net_buffer(uint32_t payload_capacity, XpBufferReply *buffer_reply);
	XaxPortBuffer *lookup_buffer(uint32_t handle);
		// exposed for copy-reduction case, where we tuck an XPBPoolElt
		// inside an XPBPacket.
	void release_buffer(uint32_t handle, bool discard);
	XPBPoolElt *lookup_elt(uint32_t handle);
};
