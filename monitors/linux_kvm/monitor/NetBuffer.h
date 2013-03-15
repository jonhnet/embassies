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

#include "pal_abi/pal_net.h"
#include "hash_table.h"
#include "MemSlot.h"
#include "NetBuffer.h"
#include "LongMessageAllocator.h"
#include "KvmNetBufferContainer.h"

class NetBuffer {
public:
	NetBuffer(MemSlot *mem_slot, LongMessageAllocation* lma);
		// initializes the ZoogNetBuffer inside the slot
		// if lma!=NULL, this owns its ref, and will drop_ref() on delete.
	NetBuffer(uint32_t guest_addr);
		// key constructor
	~NetBuffer();

	void update_with_mapped_data();	// part of ctor process.

	MemSlot *claim_mem_slot();	// part of destruction process

	uint32_t get_guest_znb_addr();
	KvmNetBufferContainer *get_container_host_addr();
	ZoogNetBuffer *get_host_znb_addr();
	uint32_t get_trusted_capacity();
		// XNB is in guest domain; its capacity field could be overwritten
		// and is thus not trustworthy.
		// This call is only valid after update_with_mapped_data,
		// if update_with_mapped_data is required (due to a lma map call
		// after ctor)
	LongMessageAllocation* get_lma();

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);

private:
	uint32_t guest_addr;	// needed for key variation
	MemSlot *mem_slot;
	LongMessageAllocation *lma;
};

