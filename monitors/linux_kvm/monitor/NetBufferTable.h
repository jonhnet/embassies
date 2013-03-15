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

class ZoogVM;

class NetBufferTable {
public:
	NetBufferTable(ZoogVM *vm, SyncFactory *sf);
	~NetBufferTable();

	NetBuffer *alloc_net_buffer(uint32_t payload_size);
	NetBuffer *install_net_buffer(LongMessageAllocation *lma, bool is_new_buffer);
	NetBuffer *lookup_net_buffer(uint32_t guest_addr);
	void free_net_buffer(NetBuffer *net_buffer);

private:
	ZoogVM *vm;
	SyncFactoryMutex *mutex;
	HashTable ht;
};
